# Morrow Designs documentation

Scans of Morrow Designs manuals, plus a few third-party datasheets for parts
used on their boards.

Most of these arrived as image-only PDFs: 400 dpi bitonal CCITT G4 scans with
no text at all. They now carry an invisible OCR text layer, so they can be
searched and `pdftotext`ed. The page images are unchanged -- the same CCITT G4
streams, byte for byte. Only text was added.

`orig/` holds the pristine pre-OCR copies. It is ignored by git; if the OCR
ever needs redoing, redo it from there rather than from an already-OCRed file.


## The slashed zero

These manuals were typed on machines that wrote zero with a slash through it:
`Ø`. Tesseract's English model has no such letter in its alphabet, so it
guessed, differently each time:

| scanned | English OCR read it as |
| --- | --- |
| `Z80` | `Z89` |
| `task 0, segment 0` | `task @, segment 8` |
| `a 40 (hex)` | `a 4@ (hex)` |
| `initialized to 0 or FF` | `initialized to 9 or FF` |
| `000050 (hex)` | `980858 (hex)` |

Every port address and hex constant in the corpus was suspect.

The fix is to OCR with the **Danish** model instead. Danish uses `Ø` as a
letter, so Tesseract reports the glyph as itself rather than guessing, and the
error becomes a single mechanical substitution.

Use `dan` alone, not `dan+eng`. With English mixed back in, its language model
re-asserts itself over numeric strings and wrecks them again (`4Ø` -> `498`,
`ØØØØ5Ø` -> `980858`). Pure `dan` is also stable at the native 400 dpi;
`dan+eng` only worked if the pages were first downsampled to ~300 dpi. English
prose comes through the Danish model cleanly -- no Danish spellings crept in.

Across the ~1320 OCRed pages this recovered about eleven thousand zeros. The
digit `0` went from 13,087 occurrences to 24,343, and `@` from 2,277 to none.


## Turning Ø back into 0

ocrmypdf renders its text layer in a `GlyphLessFont` whose ToUnicode CMap is
the identity map `<0000> <FFFF> <0000>`, so a CID *is* a Unicode codepoint.
Remapping one codepoint therefore needs no content-stream editing and moves
nothing on the page -- just append a `bfchar` block after `endbfrange`:

```
endbfrange
1 beginbfchar
<00D8> <0030>
endbfchar
```

The `bfchar` must come *after* `endbfrange`; placed before it, the range wins
and the override does nothing.


## Reproducing

Needs `ocrmypdf`, `tesseract-ocr`, `tesseract-ocr-dan`, and `python3-pikepdf`.
Note that pikepdf is the system Python's, so `slashzero.py` runs under
`/usr/bin/python3` rather than whatever `python3` resolves to.

```sh
ocrmypdf --skip-text --rotate-pages --optimize 0 --output-type pdf \
         -l dan --jobs 8  orig/FILE.pdf  /tmp/FILE.raw.pdf
/usr/bin/python3 slashzero.py /tmp/FILE.raw.pdf FILE.pdf
```

`--optimize 0` is what keeps the original image streams intact; the default
would re-encode them to JBIG2. `--output-type pdf` skips the Ghostscript PDF/A
rewrite, for the same reason. `--skip-text` leaves already-OCRed pages alone,
so the sweep can safely run over the whole directory.

The whole corpus takes about five minutes at four files in parallel.


## What is still wrong

OCR here is a finding aid. It is good enough to search on and mostly good
enough to read, but not good enough to copy values out of unchecked.

- A spurious `S` turns up inside long runs of zeros: `000050` came out
  `0000S50`. The zeros are right, there is an extra character between them.
- Genuine `@` signs are now mangled, about 100 of them, since the Danish model
  reads `@` as `€` or `e`. These are all in power specifications
  (`2.5 Amps € 115 VAC`), not in addresses. They were never turned into false
  zeros -- they became `€`, never `Ø`, so the substitution above never saw them.
- `1` is occasionally read as `l`.
- Schematic and foldout pages OCR as noise. Nothing to be done; the text layer
  on those pages is garbage and should be ignored.

Anything destined for emulator source -- port addresses, register bits, timing
constants -- should be checked against the page image.


## Documents that were already searchable

These arrived with a text layer and were left alone: the Micronix 1.61 manual,
`UM0080.pdf` / `Z80.pdf` (identical Zilog documents), the OMTI 5000 reference,
both MD-11 guides, `HDCDMA_Disk_Controller_Rev1_Apr82.pdf`, `i8259.pdf`,
`8250A-UART.pdf`, and `floppynotes.pdf`. Their text has not been checked for
the slashed-zero problem, which may well affect them too.


## slashzero.py

```python
#!/usr/bin/python3
"""Map the slashed-zero glyph (U+00D8) to '0' in an ocrmypdf text layer.

Tesseract's Danish model reports the typewriter's slashed zero as U+00D8.
ocrmypdf's GlyphLessFont uses an identity ToUnicode CMap, so we only need to
override that one codepoint -- content streams and glyph positions are untouched.
"""
import sys, pikepdf

OVERRIDE = b"""endbfrange
1 beginbfchar
<00D8> <0030>
endbfchar"""

def patch(src, dst):
    pdf = pikepdf.open(src)
    done = set()
    for obj in pdf.objects:
        try:
            if obj.get('/Subtype') != '/Type0' or '/ToUnicode' not in obj:
                continue
        except (AttributeError, TypeError, ValueError):
            continue
        st = obj['/ToUnicode']
        if st.objgen in done:
            continue
        cm = bytes(st.read_bytes())
        if b'<00D8>' in cm or b'endbfrange' not in cm:
            continue
        st.write(cm.replace(b'endbfrange', OVERRIDE, 1))
        done.add(st.objgen)
    pdf.save(dst)
    return len(done)

if __name__ == '__main__':
    n = patch(sys.argv[1], sys.argv[2])
    print(f"patched {n} ToUnicode CMap(s)")
```
