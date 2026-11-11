# ML-4 — полісинт (VST3 / AU)

4 голоси, два VCO, фільтр 12/24 дБ, дві огинаючі, LFO, режими Полі / Унісон / Акорд / Арп (арп синхронізується з темпом DAW), хорус/фленджер, пінг-понг затримка, реверб, 17 пресетів.

## Мови
- **Вікно плагіна** — українською.
- **Імена параметрів, значення й назви пресетів для хоста** — англійською латиницею. Їх показують дисплеї Maschine і доріжки автоматизації.

Порядок параметрів = сторінки Maschine по 8:

| Сторінка | Параметри |
|---|---|
| 1 | VCO1 Wave, VCO1 Octave, VCO1 Shape, VCO2 Wave, VCO2 Octave, VCO2 Shape, VCO2 Pitch, VCO2 Fine |
| 2 | Cross Mod, Ring Mod, VCO1 Level, VCO2 Level, Noise Level, Noise Color, Drive, Glide |
| 3 | Cutoff, Resonance, Key Track, Filter Slope, Mod EG Attack, Mod EG Decay, Mod EG Amount, Mod EG Target |
| 4 | Amp Attack, Amp Decay, Amp Sustain, Amp Release, LFO Wave, LFO Rate, LFO Amount, LFO Target |
| 5 | Voice Mode, Voice Depth, Arp Latch, Arp Rate, Mod FX Type, Mod FX Depth, Mod FX Rate, Mod FX Mix |
| 6 | Delay Time, Delay Feedback, Delay Mix, Reverb Time, Reverb Mix, Output |

## Збірка
Папки `ML-4/` і `.github/` кладуться в корінь репозиторію. Push запускає GitHub Actions: Windows (VST3) і macOS (VST3 + AU, universal). Готові файли — у вкладці Actions → артефакти.

Локально: `cmake -S ML-4 -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release`
