# Pisanie skryptów Python

Umieść pliki `.py` w `~/.codehex/scripts/python/`. Są automatycznie ładowane przy starcie.

## Dostępne API

```python
import codehex

codehex.log("wiadomość")
v = codehex.version()
```

## Hooki

```python
# pre_send — wywoływany przed wysłaniem
def pre_send(args: dict) -> str | None:
    text = args.get("text", "")
    codehex.log(f"Wysyłam {len(text)} znaków")
    return text  # lub None żeby nie modyfikować

# post_receive — wywoływany po otrzymaniu odpowiedzi
def post_receive(args: dict) -> None:
    text = args.get("text", "")
    codehex.log(f"Odpowiedź: {len(text)} znaków")
```

## Przykład — automatyczne zapisywanie snippetów kodu

Utwórz `~/.codehex/scripts/python/save_snippets.py`:

```python
import codehex
import re
import os

SNIPPETS_DIR = os.path.expanduser("~/.codehex/snippets")
os.makedirs(SNIPPETS_DIR, exist_ok=True)

def post_receive(args):
    text = args.get("text", "")
    blocks = re.findall(r"```(?:\w+)?\n(.*?)```", text, re.DOTALL)
    for i, block in enumerate(blocks):
        fname = os.path.join(SNIPPETS_DIR, f"snippet_{i}.txt")
        with open(fname, "w") as f:
            f.write(block)
    if blocks:
        codehex.log(f"Zapisano {len(blocks)} snippet(ów)")
```
