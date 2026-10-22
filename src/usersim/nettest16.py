#!/usr/bin/env python3
#
# nettest16 - two usersim machines, the recovered 1.6 daemons, one line
#
# Each machine gets its own root and its own pty for /dev/ttyD.  The two
# masters are bridged, so the daemons see one serial cable.  netcp on doc
# queues a transfer; after the accept timeout the daemons exchange the
# real spool record and its acknowledgement.  The bridge copies and logs
# every byte.
#
#	run:	python3 -I src/usersim/nettest16.py		recovered binaries
#		python3 -I src/usersim/nettest16.py recon	reconstruction
#		python3 -I src/usersim/nettest16.py relay	3 recovered, 2 hops
#		python3 -I src/usersim/nettest16.py relayrecon	3 reconstructed
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#
import os
import pty
import select
import shutil
import subprocess
import sys
import threading
import time
import tty

HERE = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.abspath(os.path.join(HERE, '..', '..'))
SIM = os.path.join(HERE, 'sim')
#
# recon uses the reconstructed programs (cmd/netdaemon/netdaemon and
# cmd/netcp/netcp); with no argument the recovered 1.6 binaries are the
# two machines, which is the only way to capture the distributed
# daemon's own traffic.
#
RECON = len(sys.argv) > 1 and sys.argv[1] == 'recon'
RELAY = len(sys.argv) > 1 and sys.argv[1] == 'relay'
RELAYRECON = len(sys.argv) > 1 and sys.argv[1] == 'relayrecon'
SUFFIX = '' if (RECON or RELAYRECON) else '.bin'
ND = os.path.join(TOP, 'src/micronix/cmd/netdaemon/netdaemon' + SUFFIX)
NC = os.path.join(TOP, 'src/micronix/cmd/netcp/netcp' + SUFFIX)
NETACT = os.path.join(TOP, 'src/micronix/cmd/netdaemon/netact.bin')
DOC = '/tmp/net2/doc'
SALES = '/tmp/net2/sales'
WAIT = 8 if RECON else 35

#
# These are the recovered /etc/passwd and /etc/group.  netact looks up
# the network user; without it it prints "network: No such user" and
# the delivery never happens.
#
RECOVERED_PASSWD = ('root::0:0:Super user:/:/bin/sh\n'
                    'network::9:7::/a/network:/bin/sh\n')
RECOVERED_GROUP = ('software::1:\nhardware::2:\ndocum::3:\nadmin::4:\n'
                   'sales::5:\nsupport::6:\ndaemon::7:\n')


def machine(root, me, peer):
    shutil.rmtree(root, ignore_errors=True)
    for d in ('bin', 'etc', 'dev', 'tmp', 'usr/spool/net/in',
              'usr/spool/net/out'):
        os.makedirs(os.path.join(root, d))
    for name in ('netdaemon', 'netcp'):
        src = ND if name == 'netdaemon' else NC
        dst = os.path.join(root, 'bin', name)
        shutil.copy(src, dst)
        os.chmod(dst, 0o755)
    open(os.path.join(root, 'etc/netmap'), 'w').write(me + '\n' + peer + '\n')
    open(os.path.join(root, 'etc/ttys'), 'w').write(
        'ttyD 1200 net-' + peer + '\n')
    open(os.path.join(root, 'etc/passwd'), 'w').write(
        'root::0:0:root:/:/bin/sh\n')


def line(root, dev='ttyD'):
    m, s = pty.openpty()
    tty.setraw(s)
    os.symlink(os.ttyname(s), os.path.join(root, 'dev', dev))
    return m, s


def kill_sims():
    os.system("ps -ef | grep '/usersim/sim ' | grep -E '/tmp/net[23]/' | "
              "grep -v grep | awk '{print $2}' | xargs -r kill -9")


M1 = M2 = None


def relay(seconds, to_sales, to_doc):
    end = time.time() + seconds
    while time.time() < end:
        r, _, _ = select.select([M1, M2], [], [], 0.5)
        for fd in r:
            try:
                b = os.read(fd, 4096)
            except OSError:
                continue
            if not b:
                continue
            if fd == M1:
                to_sales += b
                os.write(M2, b)
            else:
                to_doc += b
                os.write(M1, b)


def pump(m1, m2, left, right, stop):
    while not stop[0]:
        r, _, _ = select.select([m1, m2], [], [], 0.2)
        for fd in r:
            try:
                b = os.read(fd, 4096)
            except OSError:
                continue
            if not b:
                continue
            if fd == m1:
                left.extend(b)
                os.write(m2, b)
            else:
                right.extend(b)
                os.write(m1, b)


def relay_machine(root, me, body, ttys, real):
    shutil.rmtree(root, ignore_errors=True)
    for d in ('bin', 'etc', 'dev', 'tmp', 'a/network', 'usr/spool/net/in',
              'usr/spool/net/out'):
        os.makedirs(os.path.join(root, d))
    for name in ('netdaemon', 'netcp'):
        src = ND if name == 'netdaemon' else NC
        dst = os.path.join(root, 'bin', name)
        shutil.copy(src, dst)
        os.chmod(dst, 0o755)
    if real:
        dst = os.path.join(root, 'bin/netact')
        shutil.copy(NETACT, dst)
        os.chmod(dst, 0o755)
        passwd = RECOVERED_PASSWD
        group = RECOVERED_GROUP
    else:
        passwd = 'root::0:0:root:/:/bin/sh\n'
        group = ''
    open(os.path.join(root, 'etc/netmap'), 'w').write(me + '\n' + body + '\n')
    open(os.path.join(root, 'etc/ttys'), 'w').write(ttys)
    open(os.path.join(root, 'etc/passwd'), 'w').write(passwd)
    open(os.path.join(root, 'etc/group'), 'w').write(group)


def relay_test(real):
    """a -- b -- c; push a to c, then pull c to a."""
    need = [SIM, ND, NC] + ([NETACT] if real else [])
    for p in need:
        if not os.path.exists(p):
            print('missing', p)
            return 1
    a = '/tmp/net3/a'
    b = '/tmp/net3/b'
    c = '/tmp/net3/c'
    relay_machine(a, 'a', 'b c', 'ttyD 1200 net-b\n', real)
    relay_machine(b, 'b', 'a\nc',
                  'ttyD 1200 net-a\nttyE 1200 net-c\n', real)
    # c must name the route back to a, or netcp on c cannot reply
    relay_machine(c, 'c', 'b a', 'ttyD 1200 net-b\n', real)
    push = b'hello from a to c\n'
    pull = b'c motd across two hops\n'
    open(os.path.join(a, 'tmp/hello'), 'wb').write(push)
    open(os.path.join(c, 'etc/motd'), 'wb').write(pull)

    a1, s1 = line(a)
    a2, s2 = line(b)
    b1, t1 = line(b, 'ttyE')
    b2, t2 = line(c)
    c1m, c1s = pty.openpty()
    tty.setraw(c1s)
    c2m, c2s = pty.openpty()
    tty.setraw(c2s)
    c3m, c3s = pty.openpty()
    tty.setraw(c3s)
    for root, cs in ((a, c1s), (b, c2s), (c, c3s)):
        subprocess.Popen([SIM, '-d', root, '/bin/netdaemon'],
                         stdin=cs, stdout=cs, stderr=cs)

    caps = [bytearray() for _ in range(4)]
    stop = [False]
    th1 = threading.Thread(target=pump,
                           args=(a1, a2, caps[0], caps[1], stop), daemon=True)
    th2 = threading.Thread(target=pump,
                           args=(b1, b2, caps[2], caps[3], stop), daemon=True)
    th1.start()
    th2.start()
    time.sleep(1)
    wait = 45 if real else 15
    wait_pull = 100 if real else 15

    p = subprocess.run([SIM, '-d', a, '/bin/netcp',
                        '/tmp/hello', 'c%/tmp/hello'],
                       stdin=c1s, stdout=c1s, stderr=c1s, timeout=20)
    print('push', p.returncode, 'spool', os.listdir(a + '/usr/spool/net/out'))
    time.sleep(wait)
    got = os.path.join(c, 'tmp/hello')
    pushed = open(got, 'rb').read() if os.path.exists(got) else b''
    print('push arrived:', repr(pushed))

    p = subprocess.run([SIM, '-d', a, '/bin/netcp',
                        'c%/etc/motd', '/tmp/fetched'],
                       stdin=c1s, stdout=c1s, stderr=c1s, timeout=20)
    print('pull', p.returncode, 'spool', os.listdir(a + '/usr/spool/net/out'))
    time.sleep(wait_pull)
    got = os.path.join(a, 'tmp/fetched')
    fetched = open(got, 'rb').read() if os.path.exists(got) else b''
    print('pull arrived:', repr(fetched))

    stop[0] = True
    th1.join(timeout=1)
    th2.join(timeout=1)
    names = ('a -> b', 'b -> a', 'b -> c', 'c -> b')
    for name, cap in zip(names, caps):
        print('%s (%d):' % (name, len(cap)))
        print(repr(bytes(cap)))
    kill_sims()
    if pushed != push:
        print('push did not arrive')
        return 1
    if fetched != pull:
        print('pull did not arrive')
        return 1
    if b'S c /tmp/hello' not in caps[0] or b'S c /tmp/hello' not in caps[2]:
        print('push did not cross both links')
        return 1
    if b'R c /etc/motd' not in caps[0] or b'R c /etc/motd' not in caps[2]:
        print('request did not cross both links')
        return 1
    if b'S a /tmp/fetched' not in caps[3] or b'S a /tmp/fetched' not in caps[1]:
        print('reply did not cross both links')
        return 1
    return 0


def main():
    if RELAY or RELAYRECON:
        return relay_test(RELAY)
    for p in (SIM, ND, NC):
        if not os.path.exists(p):
            print('missing', p)
            return 1
    machine(DOC, 'doc', 'sales')
    machine(SALES, 'sales', 'doc')
    open(os.path.join(DOC, 'tmp/hello'), 'w').write('hello, network\n')
    global M1, M2
    M1, s1 = line(DOC)
    M2, s2 = line(SALES)
    c1m, c1s = pty.openpty()
    tty.setraw(c1s)
    c2m, c2s = pty.openpty()
    tty.setraw(c2s)

    subprocess.Popen([SIM, '-d', DOC, '/bin/netdaemon'],
                     stdin=c1s, stdout=c1s, stderr=c1s)
    subprocess.Popen([SIM, '-d', SALES, '/bin/netdaemon'],
                     stdin=c2s, stdout=c2s, stderr=c2s)
    time.sleep(2)
    p = subprocess.run([SIM, '-d', DOC, '/bin/netcp', '/tmp/hello',
                        'sales%/tmp/copy'], stdin=c1s, stdout=c1s,
                       stderr=c1s, timeout=20)
    print('netcp', p.returncode, 'spool', os.listdir(DOC + '/usr/spool/net/out'))

    to_sales = bytearray()
    to_doc = bytearray()
    relay(WAIT, to_sales, to_doc)

    open('/tmp/netdaemon16-doc-to-sales.bin', 'wb').write(to_sales)
    open('/tmp/netdaemon16-sales-to-doc.bin', 'wb').write(to_doc)
    print('doc -> sales (%d):' % len(to_sales))
    print(repr(bytes(to_sales)))
    print('sales -> doc (%d):' % len(to_doc))
    print(repr(bytes(to_doc)))
    print('spool doc', os.listdir(DOC + '/usr/spool/net/out'))
    print('spool sales in', os.listdir(SALES + '/usr/spool/net/in'))
    if RECON:
        dest = os.path.join(SALES, 'tmp/copy')
        copied = open(dest, 'rb').read() if os.path.exists(dest) else b''
        print('copied:', repr(copied))
        if copied != b'hello, network\n':
            kill_sims()
            return 1

        # The other direction: doc asks sales for /etc/motd.
        payload = b'from sales, over the wire\n'
        open(os.path.join(SALES, 'etc/motd'), 'wb').write(payload)
        mark_a = len(to_sales)
        mark_b = len(to_doc)
        p = subprocess.run([SIM, '-d', DOC, '/bin/netcp',
                            'sales%/etc/motd', '/tmp/fetched'],
                           stdin=c1s, stdout=c1s, stderr=c1s, timeout=20)
        print('netcp pull', p.returncode, 'spool',
              os.listdir(DOC + '/usr/spool/net/out'))
        relay(WAIT, to_sales, to_doc)
        print('pull doc -> sales (%d):' % (len(to_sales) - mark_a))
        print(repr(bytes(to_sales[mark_a:])))
        print('pull sales -> doc (%d):' % (len(to_doc) - mark_b))
        print(repr(bytes(to_doc[mark_b:])))
        got = os.path.join(DOC, 'tmp/fetched')
        fetched = open(got, 'rb').read() if os.path.exists(got) else b''
        print('fetched:', repr(fetched))
        if fetched != payload:
            kill_sims()
            return 1
        if b'R sales /etc/motd' not in to_sales or b'S doc /tmp/fetched' not in to_doc:
            print('pull: request or reply missing')
            kill_sims()
            return 1
    kill_sims()
    if not to_sales or b'S sales' not in to_sales or b'D!' not in to_sales:
        print('no transfer captured')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
