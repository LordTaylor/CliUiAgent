# Pisanie skryptów Lua

Umieść pliki `.lua` w `~/.codehex/scripts/lua/`. Są automatycznie ładowane przy starcie i hot-reloadowane gdy zmienią się na dysku.

## Dostępne API

```lua
-- Logowanie
codehex.log("wiadomość")

-- Wersja aplikacji
local v = codehex.version()
```

## Hooki

Zdefiniuj globalne funkcje o nazwach hooków:

```lua
-- Wywoływany przed wysłaniem wiadomości do CLI
-- args.text = tekst wiadomości
function pre_send(args)
    codehex.log("Wysyłam: " .. args.text)
    -- opcjonalnie zwróć zmieniony tekst
    return args.text .. "\n\nOdpowiadaj po polsku."
end

-- Wywoływany po otrzymaniu odpowiedzi od CLI
-- args.text = pełna odpowiedź
function post_receive(args)
    codehex.log("Otrzymano " .. #args.text .. " znaków")
end
```

## Przykład — dodanie kontekstu do promptu

Utwórz `~/.codehex/scripts/lua/add_context.lua`:

```lua
function pre_send(args)
    local enhanced = args.text .. "\n\n[Kontekst: piszę w Qt6/C++]"
    return enhanced
end
```
