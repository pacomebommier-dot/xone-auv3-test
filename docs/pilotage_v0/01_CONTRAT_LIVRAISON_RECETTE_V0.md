# XONE AUv3 — CONTRAT DE LIVRAISON ET RECETTE V0
Version 1.0 — 28 octobre 2026 — à relire avant toute déclaration « V0 prête »

## 1. Définition de la candidate
La **V0 CANDIDATE AUM** est une extension AUv3 iOS construite depuis un SHA identifié, ouvrable dans AUM après signature/installation appropriée, avec entrée et sortie MIDI réelles, et un sous-ensemble sûr des fonctions XONE déjà implémentées.

**V0 candidate** ne signifie **pas** que les essais iPhone/AUM sont PASS. L'appareil et AUM doivent être effectivement testés pour déclarer la **V0 validée AUM**.

## 2. Conditions de passage à « candidate »
Toutes les conditions suivantes doivent être satisfaites sur un même SHA :
1. Compilation et archivage iOS du vrai wrapper AUv3 (artefact identifiable, installation/signature documentées).
2. CI complète PASS : GCC, Clang, Apple Clang, ASan/UBSan, harnais de la vraie classe AU macOS, build iOS, intégrité historique.
3. Le plugin initialise ses états et accepte les événements MIDI physiques ou simulés du Xone K2, sans exiger de synthé, preset, plugin audio externe ou profil non configuré.
4. Au moins un parcours **bout en bout** `événement K2 → geste/NAV S1/S2 → décision S3 → sortie S4 → callback MIDI AU` est prouvé avec les octets et timestamps attendus.
5. Les chemins déjà raccordés nécessaires au premier usage (NAV, CC/PICKUP, SPOT, STOP/OFF, MUTE/SOLO, feedback LED configuré) restent couverts par les tests existants ou des tests ciblés au niveau AU, sans prétendre valider les destinations absentes.
6. Un port/profil/BASE/PATCH inconnu entraîne un refus local sûr : pas de mutation partielle, pas de MIDI ou d'acquittement inventés, les autres fonctions continuent.
7. Les transitions STOP, Notes OFF dues, callbacks échoués, fin de buffer et fermeture/recréation AU ne produisent pas de crash, de dette perdue ou d'ownership fantôme dans les scénarios vérifiés.
8. Une fiche de recette AUM courte et un registre des éléments non configurés sont joints.

## 3. Recette minimale sur iPhone/AUM (distincte de la CI)
- Installer la build identifiée ; vérifier la présence de XONE dans AUM et l'ouverture de son interface.
- Lancer sans profil externe et vérifier absence de crash et états explicites NON CONFIGURES.
- Ajouter l'entrée MIDI K2 ou une instance Mozaic simulatrice ; vérifier un geste NAV, un CC/pickup et un SPOT complet ON puis OFF.
- Contrôler STOP, MUTE/SOLO et le feedback LED sur les seules routes effectivement configurées.
- Sauver, fermer et rouvrir la session ; constater seulement les états dont la restauration est effectivement implémentée.
- Noter PASS / FAIL / NON TESTABLE par scénario, avec build SHA et configuration. Aucune réussite AUM ou matérielle ne peut être déduite des tests SOURCE/macOS.

## 4. Exclus de la barrière V0
Ne pas exiger avant la candidate : choix de tous les instruments, calibration des presets et FILTER/BASE, chaînes complètes PATCH/MEMORY/CUE/LOAD, PRESET/RED exhaustifs, toutes les vues LED, persistance complète, backend DJ LOOP audio, exécution Browser LOAD, ou campagnes de validation matérielle complètes.

Ces sujets restent inscrits au registre ou aux six tâches. **Reporter n'autorise pas à casser une fonction déjà opérationnelle.**

## 5. Paquet de livraison obligatoire
- Branche et commit SHA exacts ; statut de la CI et lien vers ses jobs.
- Artefact iOS identifié et consignes de signature / installation.
- Mini-matrice : `OPERATIONNEL (prouvé) / A_CONFIGURER / DIFFERE / NON_TESTE_AUM`.
- Fiche d'essais AUM et résultats **réels** lorsqu'ils existent.
- Liste courte des régressions et inconnues restantes, sans qualifier les services externes de réussis.

**STOP V0 uniquement** si un critère de sécurité ou d'ouverture/compilation échoue : traiter le défaut puis relancer les preuves correspondantes. Ne pas bloquer pour un profil futur absent.
