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
#
# vim: tabstop=4 shiftwidth=4 noexpandtab:
#
import os
import pty
import select
import shutil
import subprocess
import sys
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
SUFFIX = '' if RECON else '.bin'
ND = os.path.join(TOP, 'src/micronix/cmd/netdaemon/netdaemon' + SUFFIX)
NC = os.path.join(TOP, 'src/micronix/cmd/netcp/netcp' + SUFFIX)
DOC = '/tmp/net2/doc'
SALES = '/tmp/net2/sales'
WAIT = 8 if RECON else 35


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


def line(root, name):
    m, s = pty.openpty()
    tty.setraw(s)
    os.symlink(os.ttyname(s), os.path.join(root, 'dev/ttyD'))
    return m, s


def kill_sims():
    os.system("ps -ef | grep '/usersim/sim ' | grep '/tmp/net2/' | "
              "grep -v grep | awk '{print $2}' | xargs -r kill -9")


def main():
    for p in (SIM, ND, NC):
        if not os.path.exists(p):
            print('missing', p)
            return 1
    machine(DOC, 'doc', 'sales')
    machine(SALES, 'sales', 'doc')
    open(os.path.join(DOC, 'tmp/hello'), 'w').write('hello, network\n')
    m1, s1 = line(DOC, 'doc')
    m2, s2 = line(SALES, 'sales')
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
    end = time.time() + WAIT
    while time.time() < end:
        r, _, _ = select.select([m1, m2], [], [], 0.5)
        for fd in r:
            try:
                b = os.read(fd, 4096)
            except OSError:
                continue
            if not b:
                continue
            if fd == m1:
                to_sales += b
                os.write(m2, b)
            else:
                to_doc += b
                os.write(m1, b)

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
    kill_sims()
    if not to_sales or b'S sales' not in to_sales or b'D!' not in to_sales:
        print('no transfer captured')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
