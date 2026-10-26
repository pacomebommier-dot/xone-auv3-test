# Vérification du vrai wrapper AU — V3, 26/10/2026

Tests/test_auv3_document.mm instancie la véritable classe XoneMidiAudioUnit sous les frameworks Apple de macOS. Ce n'est pas une simulation de HostEngine ni une exécution AUM/iPhone.
Scénarios : enregistrement de sous-classe AU ; fullStateForDocument -> buffer rendu -> callback MIDI réel ; nouvelle instance depuis le document sauvegardé ; refus de données corrompues ; absence de TX au chargement ; STOP et vrais UP ; bloc obtenu avant allocation à44100 puis bus configurés48000, SHIFT90 et LONG200ms exacts.

Le harness est ajouté au job macOS/iOS après les suites C++/sanitizers, avant le build pour iphoneos. Un échec ne peut pas être ignoré : assertions actives et pipefail. Il sert à lever des incertitudes d'adaptateur sans demander une installation iPhone.
Le test CRC C++ est précisé pour changer une cellule64 en65, donc pour vérifier une corruption qui reste dans le domaine, au lieu de seulement changer UNKNOWN-2 en-1 invalide.

CI et preuves : le premier essai3e901da a passé tous les tests et le build iOS mais l'upload des artefacts a subi cinq timeouts ArtifactService/CreateArtifact. Le seul job iOS a été relancé sur le même commit ; aucune modification de code ou masquage de test pour traiter ce problème réseau.
La CI de ce nouveau harness doit être vérifiée. AUM_FULL_DEFERRED et les validations matérielles restent ouverts ; aucun de ces tests ne remplace le MIDI Learn d'un effet réel, un vrai save/reopen AUM, ou les essais K2/SH-01A.

## Corrections de l'adaptateur/harness
La première compilation543ea961 a correctement refusé un outputData nul dans l'appel de test ; le harness fournit désormais un AVAudioPCMBuffer valide.
La tentative9316f550 a compilé, puis refusé un format de test interleavé. Le harness emploie le format standard Apple (Float32 non interleavé) à48000Hz et des buffers correspondants, sans contourner le contrôle. Le silence de tous les buffers est aussi vérifié.
Le dernier LIVE est maintenant exporté via fullState ; fullStateForDocument utilise le relais standard Apple vers fullState. Le test exerce une écriture document/lecture générique, puis une écriture générique/lecture document sur une nouvelle instance. Il ne s'agit pas des snapshots PRESET/RED propres au moteur XONE, dont la sémantique et les exclusions S8 restent inchangées.
Référence Apple : https://developer.apple.com/documentation/audiotoolbox/auaudiounit/fullstatefordocument

## Profil SH-01A préparé
Profiles/SH01A_SOURCE_V1.json : 42 CC explicitement nommés par la fiche Roland officielle, sans liaison/port/canal/default/PC supposé. CC84 n'est pas inventé à partir d'un intervalle générique. Numéros, capacité TX/RX et absence SysEx sont sourcés ; valeurs discrètes et correspondances XONE restent à vérifier. Aucune émission MACHINE raccordée. Le profil reste préparatoire, L14/L16 ouverts, JU-06A séparé.
