# Branche native L02 — V0.2 en cours de recette

La V0.2 embarque une tranche C++ bornée : deux CC CH1, SurfaceInput CH15 en diagnostic, traces RX/DECISION/TX. Les 12 modules L01/11 sont préservés et testés ; leur présence ne termine pas les lots musicaux. Voir [clôture L01](docs/reprise/CLOTURE_L01.md) et [recette sans K2](docs/reprise/RECETTE_AUM_V02.md).

IPA non signé produit par la CI de cette branche ; signature et essai AUM iPhone restent distincts du build. Composant Xn02 et bundle IDs distincts de V0.1. Aucun merge main ni changement GOLDEN/S7/S8.

---

# XONE ENGINE TEST — V0.1 (prototype AUv3 MIDI pour iPad)

Date : 23 octobre 2026
État : **sources produites, transformations MIDI testées localement** ; **NON compilé sous Xcode et NON testé dans AUM sur iPad**.

## But de la V0.1

1. Installer l'application conteneur **XONE Engine Test** sur l'iPad.
2. Trouver **XONE Lab: XONE ENGINE TEST** dans les AUv3 *MIDI Processors* d'AUM.
3. Router uniquement les messages XONE:K2 à l'entrée de l'instance.
4. Vérifier que deux commandes MIDI ressortent sur **XONE TEST OUT**.

Il s'agit d'un plugin MIDI **`aumi`**, sans synthétiseur et sans traitement audio.

### Deux fonctions exactes

| MIDI entrant de XONE:K2 | MIDI sortant de XONE TEST OUT |
| --- | --- |
| CC n°4, n'importe quel canal | CC n°74, canal MIDI 1, **même valeur 0–127** |
| CC n°8, n'importe quel canal | CC n°71, canal MIDI 1, **même valeur 0–127** |

Les autres messages ne sont **pas** transmis (volontairement pour ce test). Pas de retour LED, pas de sauvegarde, pas de transport ni de PATCH dans cette version. Ces CC sont choisis à partir du mapping de travail XONE, à vérifier avec le K2 connecté.

## Dossiers

- `App/` : toute petite application iPad qui contient et installe l'extension.
- `Extension/` : composant AUv3 de type MIDI Processor + vue de diagnostic.
- `Shared/XoneMidiTransform.c` : les deux commandes MIDI (moteur C réutilisable).
- `Tests/` : tests unitaires du moteur MIDI exécutables sur Windows (avec compilateur C), macOS ou Linux.
- `project.yml` : configuration XcodeGen pour produire le projet Xcode.

## Installer sur iPad : chemin le plus simple (Mac + Xcode)

### Prérequis

- Accès ponctuel à un **Mac** compatible avec une version de Xcode prenant en charge ton iPadOS.
- Xcode (gratuit, fourni par Apple).
- Un identifiant Apple ajouté à Xcode (`Settings > Accounts`). **Un compte Apple Developer payant n'est pas nécessaire pour ce test sur son propre iPad.** Le profil gratuit de développement est limité dans le temps et peut nécessiter une réinstallation après 7 jours.
- Le K2, l'iPad et AUM.

### Étapes détaillées

1. Copie le dossier `XoneEngineTest_V0_1` sur le Mac, puis ouvre le Terminal dans ce dossier.
2. Installe XcodeGen si nécessaire (`brew install xcodegen` si Homebrew est installé). XcodeGen : https://github.com/yonaskolb/XcodeGen
3. Lance `xcodegen generate`. Cela crée `XoneEngineTest.xcodeproj`.
4. Ouvre ce `.xcodeproj` dans Xcode.
5. Dans **Signing & Capabilities**, configure ton équipe Apple pour **les deux cibles** : `XoneEngineTest` (application) et `XoneMidiAU` (extension). Laisse la signature automatique.
6. Si les identifiants `org.xonelab.XoneEngineTest` et `org.xonelab.XoneEngineTest.XoneMidiAU` ne sont pas disponibles pour ta team, remplace leurs valeurs **dans `project.yml`** par des identifiants uniques, en conservant l'ID de l'extension préfixé par l'ID de l'application, puis regénère le projet.
7. Branche et déverrouille ton iPad. Accepte « Faire confiance », active le mode développeur iPadOS si demandé, et choisis-le comme destination de construction dans Xcode.
8. Sélectionne la **cible/application `XoneEngineTest`** et clique sur **Run**. Xcode doit construire et installer l'app **et l'extension embarquée**.
9. Lance une fois `XONE Engine Test` sur l'iPad pour confirmer l'installation de l'application conteneur.
10. Ferme puis rouvre AUM. Crée une **tranche MIDI** (pas une tranche audio), sélectionne un AUv3 MIDI Processor, cherche `XONE Lab: XONE ENGINE TEST`.
11. Charge l'instance. L'interface doit indiquer `XONE ENGINE · TEST 0.1`.
12. Dans la matrice de routage MIDI AUM, connecte **uniquement** la sortie MIDI du XONE:K2 à l'entrée du plugin. **Ne connecte pas un retour LED au K2** pour ce test.
13. Route la sortie **XONE TEST OUT** vers un moniteur MIDI (ou vers un synthé avec CC74 / CC71 MIDI-learn), puis manipule des contrôles du XONE qui émettent CC4 et CC8.
14. Vérifie que le moniteur reçoit **CC74** et **CC71**, sur **canal 1**, avec la valeur identique à l'entrée.

### Résultats / critères de validation

- **I — Installation** : l'icône de l'app et son écran sont visibles sur l'iPad.
- **A — Reconnaissance** : le plugin est présent dans les *MIDI Processors* d'AUM et s'ouvre.
- **M — MIDI** : deux remappings CC ont été observés sur un moniteur MIDI de sortie.

C'est uniquement après ces trois validations qu'on ajoutera une commande de LED, puis le retour du transport hôte.

### Si le plugin n'apparaît pas dans AUM

1. Vérifier dans Xcode que le produit `XoneEngineTest.app` intègre `PlugIns/XoneMidiAU.appex`.
2. Contrôler les bundle identifiers (l'extension doit partager le préfixe de l'app), la signature et les journaux d'installation.
3. Dans `Extension/Info.plist`, vérifier `NSExtensionPointIdentifier = com.apple.AudioUnit-UI` et `AudioComponents.type = aumi`.
4. Fermer complètement AUM, relancer l'app conteneur, puis ouvrir AUM.
5. S'assurer de chercher dans **MIDI Processor** sur une **tranche MIDI**.

### Si le plugin apparaît mais ne produit pas de CC

1. Dans AUM, contrôler que le K2 transmet bien le CC4 ou CC8 vers le plugin : utiliser le moniteur MIDI de l'entrée si nécessaire.
2. Vérifier que le port **XONE TEST OUT** est raccordé à la destination voulue, pas à l'entrée du XONE.
3. Ne pas supposer que l'entrée XONE est sur le canal 1 : notre V0.1 accepte tous les canaux entrants.
4. Récupérer un éventuel message d'erreur Xcode ou une capture du routage AUM avant de modifier le code.

## Alternative sans Mac

Des outils de compilation iOS utilisant Linux existent (par exemple `xtool` + SDK iOS), potentiellement exécutables dans WSL2 sur Windows. **Ce dossier ne fournit pas de chaîne WSL2/xtool validée**, donc ce n'est pas la procédure conseillée pour la première installation. Un `.zip` de sources n'est pas une application iOS installable : il faut encore compiler et signer le bundle iOS.

## Limites du prototype

- L'exclusivité « XONE uniquement » est assurée **par le routage AUM**, pas par une authentification du fabricant MIDI au niveau du plugin. Cette dernière exigence sera étudiée plus tard.
- Les sorties ne sont pas un accès privé aux paramètres d'AUM : elles utilisent le routage et le MIDI Learn d'AUM.
- Le code du rendu AUv3 respecte l'intention temps réel (aucune écriture disque/log/réservation dynamique dans la callback), mais il n'a **pas encore été validé par un compilateur Apple** et pourra nécessiter des ajustements avant l'étape I/A.

### Tests réalisés ici

`Tests/run.sh` : 10 cas réussis sur moteur C (CC4, CC8, valeurs limites, canaux entrants divers, messages ignorés, invalides). Cela **ne constitue pas une validation AUv3**.

## Références techniques

- AUM User Guide : https://www.kymatica.com/aum/help
- Apple Audio Unit Extensions : https://developer.apple.com/documentation/avfaudio/creating-an-audio-unit-extension
- Apple AUAudioUnit : https://developer.apple.com/documentation/audiotoolbox/auaudiounit
- AudioKit MIDI AUv3 tutorial : https://audiokitpro.com/auv3-midi-tutorial-part1/
