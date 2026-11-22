# NOIRNUIT REVERSE — VST3 (prototype fonctionnel v1.0)

Effet original de **reverse reverb** pour voix et instruments, développé en **C++ / JUCE**.
Compatible VST3 Windows x64 après compilation. Une application standalone est aussi incluse dans la configuration du projet. Inspiré du *flux de travail* d'AtmosRiser de WaveAtmos, **sans reprendre son code, ses fichiers, sa marque ou son interface graphique**. Aucune affiliation avec WaveAtmos.

## Fichiers fournis

- `Source/ReverseEngine.*` : moteur DSP stéréo indépendant de JUCE, **testé ici**.
- `Source/PluginProcessor.*` : capture audio de la piste, lecture du BPM, paramètres, import de fichiers, préécoute, WAV 24 bits, sauvegarde d'état.
- `Source/PluginEditor.*` : interface graphique, 4 macros, quatre effets, presets, drag & drop de fichier et export.
- `CMakeLists.txt` : génération VST3 et standalone avec JUCE.
- `BUILD_WINDOWS.bat` : compilation Windows et copie du VST3 dans le répertoire système.
- `Tools/OfflineRenderer.cpp` : outil en ligne de commande pour tester le moteur sans VST3.
- `Tests/test_engine.cpp` : tests automatiques DSP.
- `Demo/` : prise de test synthétique (ne contient aucune voix enregistrée ou protégée) et reverse exporté.

## Installation et compilation sur Windows 10/11

**Il n'y a pas de `.vst3` Windows déjà compilé dans cette archive.** Le build nécessite Windows et les outils ci-dessous.

1. Installer **Visual Studio 2022 Community** avec la charge de travail **Développement Desktop en C++** et le SDK Windows.
2. Installer **Git** et **CMake** (et ajouter ces programmes à `PATH`).
3. Extraire le ZIP dans un dossier simple, par exemple `C:\NoirnuitReverse`.
4. Double-cliquer sur `BUILD_WINDOWS.bat` (connexion Internet requise au premier lancement pour télécharger JUCE 8.0.13).
5. Le script installe le dossier `.vst3` compilé dans `C:\Program Files\Common Files\VST3` si les droits le permettent. Sinon, copier manuellement le dossier `.vst3` trouvé dans `build/NoirnuitReverse_artefacts/Release/VST3/`.
6. Dans FL Studio : **Options > Manage plugins > Find installed plugins**. Rechercher `NOIRNUIT REVERSE` dans les effets.

Pour compiler manuellement :

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --target NoirnuitReverse_VST3
```

Si Visual Studio 2022 n'est pas disponible, choisir dans CMake un générateur Windows compatible avec vos outils C++ (Visual Studio 2026, par exemple). JUCE utilise des licences open-source et commerciales : vérifiez les conditions sur https://juce.com/legal/ avant de distribuer un plugin fermé.

## Utilisation dans FL Studio

1. Insérer **NOIRNUIT REVERSE** sur une piste contenant une voix ou un instrument.
2. Lire le passage à transformer (la capture audio tourne en continu pendant la lecture). Choisir **MODE VOIX** pour le de-esser automatique ; le désactiver pour instruments/percussions.
3. Option : cliquer sur **CAPTURER** pour figer l'audio entendu, ou **IMPORTER AUDIO** pour charger directement un fichier WAV/AIFF/MP3. **ENTRÉE LIVE** réactive le flux live.
4. Choisir **1, 2, 4 ou 8 mesures** (4/4, BPM du projet ; défaut 120 BPM si le transport ne le fournit pas).
5. Régler `DECAY`, `TONE`, `WIDTH`, `DE-ESS`. Activer facultativement `PHONE`, `CHORUS`, `DRIVE` ou `TREMOLO`.
6. Cliquer sur **CRÉER LE REVERSE**. Le rendu s'effectue hors du thread audio, et le résultat est prêt en WAV stéréo 24 bits.
7. Cliquer **ÉCOUTER**, puis lancer la lecture du transport FL Studio pour préécouter le son sur cette piste. Le signal de la piste reste transmis normalement hors préécoute.
8. Faire glisser **GLISSER WAV** dans la playlist de FL Studio ; si le glisser-déposer n'est pas pris en charge par le wrapper du DAW, utiliser **EXPORTER WAV**, puis importer le fichier manuellement. Placer la **fin** de la montée juste avant le début de la phrase visée.

Le bouton **+ PRESET** écrit un fichier `.nnr`. **IMPORT PRESET** ouvre un preset. Quatre presets d'usine : Vocal/Clean, Dream/Wide, Dark/Telephone, Glitch/Rhythm. Les paramètres sont également sauvegardés avec le projet FL Studio.

## Traitement et limites de cette v1

Le moteur repère l'activité récente dans le clip, prend jusqu'à ~2,25 secondes de matière, la lit à l'envers, la diffuse dans une réverbération à réseau 8 délais (FDN), inverse le résultat et effectue une finition synchronisée avec la durée du BPM. Le de-esser analyse l'énergie haute fréquence et diminue les sibilances de manière adaptative. Aucun composant logiciel de WaveAtmos n'est utilisé.

- **La qualité et la sonorité ne sont pas identiques à AtmosRiser** : les algorithmes internes du produit commercial sont inconnus.
- Synchronisation basée sur **4 temps/mesure** ; les signatures différentes nécessitent une adaptation.
- L'export est limité à 45 s (maximum de sécurité) et la capture live conserve jusqu'à 45 s, avec 12 s sélectionnées par défaut.
- Le fichier importé conserve jusqu'à ses 15 dernières secondes ; l'algorithme cherche la portion active la plus récente.
- Le moteur DSP est testé localement, **mais le plugin JUCE/VST3 n'a pas pu être compilé/testé sur Windows dans l'environnement de génération**. Les détails de compatibilité FL Studio, du rendu UI et du glisser-déposer nécessitent une validation Windows réelle.
- Le glisser-déposer de fichiers hors des fenêtres de plugins varie selon la version du DAW, le wrapper et les permissions Windows. **EXPORTER WAV** reste disponible.
- Les fichiers sources audio importés ne sont pas incorporés dans l'état du projet ; ils doivent être rechargés si le DAW est rouvert.
- **Pas de génération en temps réel** : le plugin analyse un passage, puis produit un WAV de montée à placer dans la playlist.

## Test moteur sans installer JUCE

Avec GCC/Clang (Linux/macOS) :

```bash
g++ -std=c++17 -O3 Source/ReverseEngine.cpp Tools/OfflineRenderer.cpp -o noirnuit-render
./noirnuit-render Demo/source_demo.wav reverse.wav 120 4 Dream

g++ -std=c++17 -O2 Source/ReverseEngine.cpp Tests/test_engine.cpp -o reverse-tests
./reverse-tests
```

Avec le rendu CLI, le format source doit être WAV PCM16 ou float32, mono/stéréo. Les fichiers générés sont WAV stéréo PCM16 ; le VST exporte en 24 bits. Les presets CLI sont `Vocal`, `Dream`, `Dark`, `Glitch`.

## Licence

Tout le code applicatif écrit dans ce dossier est original. Vous pouvez le modifier pour vos productions. Toute redistribution publique ou commerciale combinée avec JUCE doit respecter la licence JUCE applicable. Les marques commerciales AtmosRiser et WaveAtmos appartiennent à leurs propriétaires respectifs.
