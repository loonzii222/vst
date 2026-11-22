# Compiler le VST3 Windows sans installer Visual Studio

Cette archive contient une configuration GitHub Actions (`.github/workflows/compiler-vst3-windows.yml`) qui demande à une machine Windows hébergée par GitHub de compiler le plugin. **Le fichier `.vst3` Windows n'est pas fourni et cette procédure n'a pas été exécutée sur GitHub.** Un compte GitHub et l'accès à GitHub Actions sont nécessaires.

1. Ouvre https://github.com/new et crée un dépôt (par exemple `noirnuit-reverse`). Évite de publier le code si tu ne souhaites pas qu'il soit public : choisis **Private**.
2. Décompresse cette archive et transfère **tout le contenu du dossier racine** dans le dépôt, y compris le dossier caché `.github` et son fichier `.github/workflows/compiler-vst3-windows.yml` (le fichier doit être à la racine du dépôt dans `.github/workflows/`). Sur GitHub, on peut utiliser **Add file > Upload files** ; vérifie que le dossier `.github` a bien été transféré, car certains outils masquent les dossiers cachés. Tu peux aussi utiliser GitHub Desktop pour une copie fiable.
3. Le commit sur la branche `main` déclenche automatiquement le workflow. Sinon, ouvre **Actions > Compiler NOIRNUIT REVERSE VST3 (Windows) > Run workflow**.
4. Si toutes les étapes sont vertes, ouvre le détail de l'exécution, puis télécharge dans **Artifacts** le fichier **NOIRNUIT_REVERSE_Windows_x64**. Son contenu comprend `NOIRNUIT_REVERSE_Windows_x64.zip`.
5. Décompresse cet archive intérieur et copie le dossier `.vst3` dans `C:\Program Files\Common Files\VST3`.
6. Dans **FL Studio**, ouvre **Options > Manage plugins**, lance un nouveau scan avec **Find installed plugins**, puis recherche **NOIRNUIT REVERSE**.

Si l'action **Compiler le VST3** échoue, copie le message d'erreur de GitHub Actions dans ChatGPT pour corriger le code et relancer le build.

**Limites** : l'interface graphique, la compatibilité avec FL Studio et le glisser-déposer ne sont pas encore vérifiés sur Windows. GitHub télécharge JUCE lors de la compilation ; une connexion et des autorisations de réseau sont nécessaires. Vérifier les termes de licence JUCE en cas de distribution.
