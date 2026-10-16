# Grit

Plugin di rumore e glitch originale, costruito con JUCE 8 (VST3 + Standalone).

## Compilazione (Windows, Visual Studio 2022)
    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Release

Il VST3 si trova in `build/Grit_artefacts/Release/VST3/Grit.vst3`
(copialo in `C:\Program Files\Common Files\VST3\`). Il primo `cmake -B build`
scarica JUCE da GitHub, quindi serve la rete.

## Parametri
- Input / Output: guadagno in dB (±24)
- Mix: dry/wet
- Chaos: quanto il modulatore casuale agisce su riduzione di sample rate (1-64 campioni),
  flutter di ampiezza
- Speed: frequenza (0.1-50 Hz) con cui il modulatore estrae un nuovo valore casuale
- Noise: rumore bianco che segue l'inviluppo del segnale
- Gate: soglia; sotto di essa l'effetto si dissolve nel segnale pulito (Off = sempre attivo)

## Struttura del DSP (per campione)
1. Input gain
2. Modulatore random (sample & hold + slew 5 ms)
3. Sample & hold sul segnale con lunghezza casuale (chaos)
4. Flutter di ampiezza + rumore, poi tanh
5. Gate: crossfade wet/dry in base all'inviluppo
6. Mix e output gain
