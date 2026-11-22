# NOIRNUIT REVERSE PRO — mise à jour sur GitHub

Le dépôt existant est : https://github.com/loonzii222/vst

## Méthode rapide (GitHub, sans Visual Studio)

1. Extraire `NOIRNUIT_REVERSE_PRO_V2_MISE_A_JOUR.zip` sur ton PC.
2. Ouvrir https://github.com/loonzii222/vst et rester sur la branche `main`.
3. Cliquer **Add file → Upload files**.
4. Depuis le dossier extrait, glisser les dossiers `Source` et `Assets`, puis les fichiers `CMakeLists.txt`, `README_FR.md` et `MISE_A_JOUR_GITHUB.md`. Tu peux aussi sélectionner tout le contenu du dossier extrait. Ne glisse PAS le ZIP lui-même.
5. GitHub doit indiquer les fichiers `Source/PluginEditor.cpp`, `Source/PluginEditor.h` et `CMakeLists.txt` comme modifiés, et `Assets/noirnuit_surface.png` comme nouveau fichier. Vérifie les chemins avant validation.
6. Cliquer **Commit changes** et confirmer sur `main`.
7. Aller dans l'onglet **Actions**, ouvrir la nouvelle exécution de `Compiler NOIRNUIT REVERSE VST3 (Windows)` (ou lancer `Run workflow` si nécessaire).
8. Si l’exécution est **verte**, ouvrir la section **Artifacts**, télécharger `NOIRNUIT_REVERSE_Windows_x64`, puis décompresser le ZIP contenu pour obtenir `NOIRNUIT REVERSE.vst3`.
9. Fermer FL Studio, remplacer l’ancienne version du dossier VST3 dans `C:\Program Files\Common Files\VST3` et relancer **Options → Manage plugins → Find installed plugins**.

Si GitHub refuse de remplacer un fichier par upload, ouvre-le dans le dépôt, clique sur le crayon **Edit**, remplace son contenu par le fichier local, puis confirme. Pour l’image PNG, utiliser **Add file → Upload files** dans le dossier `Assets` (ou GitHub Desktop pour copier les dossiers facilement).

**Important :** ton projet original reste sauvegardé dans l’historique GitHub. La v2 modifie l’affichage uniquement, mais sa compilation et son comportement UI Windows restent à valider sur ton runner GitHub.
