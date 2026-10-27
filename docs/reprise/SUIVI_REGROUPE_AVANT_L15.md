# Suivi regroupé — six tâches avant la validation AUM L15

Décision utilisateur du 27 octobre 2026 : regrouper les petites reprises en fonctions complètes, écrire le suivi et annoncer la tâche courante sur le total restant.

Branche : `feature/native-e3-bar-beat-reprise`. Base de code vérifiée : `52e518b`, documentation de preuve `043d47c`. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40521144053.

Ce suivi organise le travail restant dans les lots natifs existants ; il ne remplace ni les contrats musicaux ni le plan des 8 étapes / 16 lots. Les six regroupements remplacent l’estimation de 10 à 15 petites reprises. Ils sont plus larges et de durées différentes ; leur nombre ne constitue pas une estimation calendaire.

## Position courante

- Tâches de développement validées automatiquement : **0/6** dans ce nouveau suivi.
- Tâches de développement restantes : **6**, tâche courante comprise lorsqu’elle démarre.
- Tâche suivante : **1/6 — Actions et sorties musicales restantes**. Statut : À DÉMARRER ; aucun code nouveau de cette tâche n’est revendiqué par la création du suivi.
- Travail déjà acquis avant ce compteur : surface/S2/NAV, CC/pickup/LIVE, LOOP, SPOT SMART/MIX/TOUCH/FLASH, horloge et MUTE/SOLO/BAR, LED MUTE et restitution des sélecteurs BASE NAV, état AU du dernier LIVE. Les réserves et preuves restent dans leurs reprises spécialisées.
- L15 = campagne de validation AUM après ces six regroupements. L16 = essais matériels ensuite. Aucun PASS AUM ou matériel à ce stade.

## Les six tâches, dans l’ordre

| N° | Fonction à livrer | Limite concrète et preuve attendue | État |
|---|---|---|---|
| 1/6 | Actions et sorties musicales restantes — E3, L05/L06 | Raccorder les actions TOP/BOT/DJ encore absentes au vrai dispatcher AU et à l’API S7, avec quantification contractuelle, captures de destination, OFF dus, STOP, collisions et callbacks échoués. La transition vers une destination non prête reste refusée ; son intégration aux owners S8 relève de 3/6. Pas de réécriture de MUTE, CC ou SPOT déjà raccordés. | À DÉMARRER |
| 2/6 | Chaîne PATCH / MEMORY / CUE / LOAD / BASE — E4, L07/L08 | Raccorder les modules de banques existants aux gestes et décisions réels : save/recall, destination et BASE validées, protections contre rappel incomplet, règles de capture et UNKNOWN/INVALID conservées. Vérifier les octets musicaux produits et les interactions avec 1/6. La persistance entre instances AU relève de 5/6. | À DÉMARRER |
| 3/6 | PRESET / RED et propriétaires S8 — E5, L09/L10 | Raccorder le rappel cohérent, les alternatives exclusives et transitions de propriétaire aux banques et à BAR. Fermer les anciens owners/OFF avant la nouvelle destination ; appliquer les arbitrages C01/C03/C04 existants. Restaurer les états MUTE/SOLO selon le contrat et annuler les demandes en attente concernées. Exclure les modes SPOT MACHINE des captures PATCH/PRESET/RED. | À DÉMARRER |
| 4/6 | Feedback et ensemble des vues LED — E6, L11/L12 | Raccorder les observations authentifiées et les vues restantes, dont BASE FX/MEMORY, PRESET/RED et contextes associés, en utilisant les données devenues disponibles en 2/6 et 3/6. Préserver UNKNOWN et séparer désiré, envoyé et observé ; tests des octets MIDI15, pending/applied, refus, STOP, sorties de page et reconnexion. | À DÉMARRER |
| 5/6 | Persistance complète et restauration sûre — E7, L13/L14 | Étendre l’état AU existant aux domaines/NAV/banques autorisés par leurs contrats ; vérifier save/recreate et compatibilité des données. Raccorder le replay seulement vers une destination et un profil validés, avec owners/OFF et sans inventer une confirmation matérielle. Les valeurs et modes exclus des snapshots restent exclus. | À DÉMARRER |
| 6/6 | Intégration finale et paquet de validation AUM — préparation L15 | Exercer les parcours combinés 1–5 dans le vrai wrapper, corriger leurs régressions, obtenir la CI complète sur le SHA à tester. Préparer le build iOS, les routes AUM/API/LED/profils vérifiées sur source et une fiche courte d’essais avec résultats attendus. Documenter les inconnues ; aucun routage audio dans XONE. Livrer un paquet identifiable pour L15, sans prétendre avoir exécuté la campagne AUM. | À DÉMARRER |

## Cadence et annonces

Au début de chaque regroupement, annoncer par exemple :

> **Tâche 1/6 en cours — Actions et sorties musicales restantes. 6 tâches restantes avant L15, celle-ci comprise.**

Après sa validation automatique :

> **Tâche 1/6 validée automatiquement — commit …, CI … . 5 tâches restantes avant L15. Prochaine : 2/6 — Banques et rappels.**

Le numéro reste stable pendant les corrections, les sous-tranches et les multiples commits du même regroupement. Une correction de CI, un test manquant ou une mise à jour documentaire ne crée pas une nouvelle tâche numérotée. Actualiser ici le statut, la preuve et la position courante à la fin de chaque regroupement ; ne pas compter les commits comme des tâches.

Un regroupement peut avoir plusieurs commits cohérents. Il est validé automatiquement lorsque son parcours est intégré au vrai AU, ses scénarios de régression passent, les contrôles historiques restent PASS et la CI concernée est PASS (GCC/Clang, Apple Clang, ASan/UBSan, wrapper AU macOS et build iOS selon le périmètre). Un module isolé ou une CI en attente ne suffit pas. Cela n’est pas une validation AUM.

Lire seulement les contrats et sources GOLDEN/S7/S8 utiles au regroupement en cours. Inventorier les actions manquantes au début de 1/6 dans ce seul périmètre ; ne pas rouvrir L01–L16. Les LED FX/MEMORY attendent 4/6 plutôt qu’une série de reprises partielles avant leurs banques/owners. Les dépendances entre regroupements se traitent dans leur tâche propriétaire ; ne pas inventer de valeur ou de comportement pour avancer artificiellement le compteur.

Si une dépendance bloquante démontrée impose de modifier ce découpage, l’indiquer ici avec sa cause et annoncer explicitement le nouveau total. Ne pas changer le dénominateur silencieusement, ni conserver un bloc faussement terminé. Aucun merge main ; archives et empreintes protégées intactes. Les essais AUM et matériels restent aux jalons L15/L16, sauf demande humaine explicite.

## Preuves des regroupements

| Tâche | Commit(s) fonctionnel(s) | CI | Réserves avant L15 |
|---|---|---|---|
| 1/6 | — | — | Non commencée |
| 2/6 | — | — | Non commencée |
| 3/6 | — | — | Non commencée |
| 4/6 | — | — | Non commencée |
| 5/6 | — | — | Non commencée |
| 6/6 | — | — | Non commencée |