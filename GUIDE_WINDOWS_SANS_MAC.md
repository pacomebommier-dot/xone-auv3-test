# XONE ENGINE V0.1 — guide Windows 11 -> iPad -> AUM

## Ce qui est fourni / non fourni

Ce dossier contient le code du prototype AUv3 MIDI (`aumi`) et un workflow GitHub Actions permettant de **tenter sa compilation à distance sur macOS**. Le code natif n'a pas encore été compilé sous Xcode ; on ne peut donc pas garantir que le premier essai aboutira sans correction. L'archive ZIP de ce dossier n'est **pas** un `.ipa` installable.

## 1. Créer un dépôt GitHub public temporaire (sans mot de passe Apple)

1. Sur Windows, télécharger puis **extraire** le ZIP de projet.
2. Ouvrir https://github.com/ ; se connecter ou créer un compte gratuit.
3. Ouvrir https://github.com/new ; indiquer `xone-auv3-test`, choisir **Public** et cocher l'initialisation avec README (GitHub doit vous laisser utiliser « Add file »). Ne publier aucun mot de passe, certificat, donnée personnelle ou script privé.
4. Dans le dépôt, `Add file` -> `Upload files`.
5. Déposer **tout le contenu à l'intérieur** de `XoneEngineTest_V0_1`, et non le ZIP lui-même. Vérifier que `project.yml` figure à la racine et que `.github/workflows/build-ios-unsigned.yml` existe (le dossier `.github` est important).
6. En bas, `Commit changes`.

**ATTENTION** : Un dépôt public est visible par tous. Celui-ci ne doit contenir que le prototype de démonstration. Pas de contrats XONE privés, pas de fichiers AUM, pas d'identifiants Apple.

## 2. Lancer la compilation sur GitHub

1. Ouvrir l'onglet `Actions` du dépôt et activer les workflows si GitHub le demande.
2. Choisir `XONE V0.1 - IPA iPad (non signe)`.
3. Cliquer `Run workflow`, choisir la branche principale (`main`), puis confirmer.
4. Ouvrir la nouvelle exécution. Si le statut est **vert**, descendre à la section `Artifacts` et télécharger `XoneEngineTest_V0_1_IPA`.
5. Extraire ce ZIP d'artifact : il contient **`XoneEngineTest_V0_1_UNSIGNED.ipa`**. Ne pas confondre ce `.ipa` avec le ZIP initial des sources.
6. Si le statut est **rouge**, ouvrir l'étape en erreur et relever **son nom + les premières lignes d'erreur**. Il s'agit d'un prototype non compilé ; corriger l'erreur est la suite normale du test.

GitHub propose gratuitement l'exécution des runners standard pour les dépôts publics. Vérifier ses propres paramètres de facturation avant tout usage dans un dépôt privé.

## 3. Préparer Windows et l'iPad

1. Sauvegarder si souhaité les données de l'iPad, le déverrouiller et le relier à Windows via un câble **avec données USB**.
2. Installer Sideloadly **uniquement depuis** https://sideloadly.io/ (Windows).
3. Suivre le site officiel pour l'installation éventuelle des versions requises d'iTunes et d'iCloud provenant directement d'Apple, pas nécessairement celles du Microsoft Store.
4. Sur iPad, accepter `Faire confiance à cet ordinateur` si demandé.

## 4. Signer et installer le fichier IPA

1. Lancer Sideloadly sur Windows.
2. Dans `iDevice`, choisir l'iPad connecté.
3. Déposer `XoneEngineTest_V0_1_UNSIGNED.ipa` dans Sideloadly.
4. Choisir `Apple ID Sideload` (mode de signature par compte Apple). Utiliser un identifiant Apple valide ; aucun compte développeur payant n'est nécessaire pour un essai personnel standard. Ne communiquer ce mot de passe ni à GitHub, ni dans cette conversation.
5. Dans `Advanced Options`, **laisser décoché `Remove App Extensions (Plug-ins)`**, car notre AUv3 est précisément une app extension. Ne pas retirer les PlugIns.
6. Lancer `Start`, puis suivre les demandes de connexion / confirmation Apple.
7. Sur iPad, si nécessaire : `Réglages > Confidentialité et sécurité > Mode développeur` (activation et redémarrage), puis `Réglages > Général > VPN et gestion de l’appareil` pour approuver le développeur.
8. Chercher l'icône **XONE Engine Test**, l'ouvrir et vérifier l'écran `Application installée`.

Avec la signature gratuite, l'application expire au bout d'environ **7 jours** ; elle doit alors être resignée/rafraîchie. La signature de l'extension elle-même doit rester valide et conservée. Sideloadly peut resigner des extensions, mais le comportement précis avec notre nouveau composant AUv3 n'a pas encore été validé sur iPad.

## 5. Tester la présence dans AUM

1. Quitter complètement AUM s'il est déjà ouvert, puis le relancer.
2. Ajouter une **tranche MIDI** (et non audio).
3. Dans les **MIDI Processors AUv3**, chercher `XONE Lab: XONE ENGINE TEST`.
4. Charger l'instance et vérifier que la fenêtre affiche `XONE ENGINE · TEST 0.1`.
5. Si le plugin apparaît : succès de la validation installation + découverte. Ensuite seulement brancher XONE et tester les CC.

## 6. Tester les deux messages XONE

1. Connecter **uniquement** le port MIDI XONE:K2 à l'entrée du plugin dans la matrice MIDI AUM.
2. Connecter la sortie MIDI `XONE TEST OUT` à un moniteur MIDI.
3. Envoyer CC n°4 ou CC n°8 depuis le XONE (sur n'importe quel canal).
4. Vérifier la réception sur **canal 1** des messages : `CC4 -> CC74` et `CC8 -> CC71`, valeurs inchangées.
5. Il n'y a **aucun retour LED** dans cette V0.1 ; ne pas envoyer le plugin vers XONE pour le moment.

## Dépannage rapide

- Pas de `Actions > Run workflow` : vérifier le fichier `.github/workflows/build-ios-unsigned.yml` sur la branche par défaut.
- Compilation rouge : prendre le journal de l'étape en échec. Ne pas acheter un programme développeur Apple pour corriger une erreur de compilation.
- Sideloadly ne voit pas l'iPad : vérifier confiance USB, câble de données et composants Apple installés sur Windows.
- Application installée mais plugin absent d'AUM : vérifier dans l'IPA la présence de `Payload/XoneEngineTest.app/PlugIns/XoneMidiAU.appex` ; ne pas cocher `Remove App Extensions`; relancer l'app et AUM.
- Extension présente mais AUM ne la voit pas : cela peut être une limite de signature, de déclaration AUv3, ou du code lui-même ; il faut récupérer les logs et diagnostiquer avant de conclure.

Sources : GitHub Docs (Actions et upload), https://sideloadly.io/, https://kymatica.com/aum/help
