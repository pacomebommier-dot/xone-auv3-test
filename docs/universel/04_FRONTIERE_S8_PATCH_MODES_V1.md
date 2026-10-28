# XONE UNIVERSEL — S8 / PATCH locaux — note d'implémentation native V1
Date : 28 octobre 2026. Branche candidate PR #7, sur PR #6. **Pas un nouveau contrat qui supplante S7/S8.**

## Autorités inchangées
- S1/S2 : DOWN/UP et qualification du geste ; S3 : NAV, scheduling, seul écrivain LIVE ; S4 : MIDI virtuel historique sans backend musical imaginaire.
- S8 : paramètres d'une alternative instrumentale, configuration SPOT par instrument/voix, trois PATCH locaux **initialement vides**, BASE distincte du quatrième TOP.
- PATCH/BASE/PRESET/RED ne copient, ne modifient et ne restaurent **jamais** les modes SPOT SAMPLE/MACHINE. Les modes FX/INIT restent un domaine distinct.
- Aucun chargement de programme Roland, MIDI Learn, AUM Strip, CC externe, valeur RESET_DEFAULT ou preuve d'ACK matériel n'est présumé.

## Implémentation native de la frontière S8
- `XoneS8SamplePatchBank.hpp` : huit samples XONE, trois slots chacun, **28 paramètres PATCH** : 16 CC + 12 états SPOT 0/1. Douze modes de configuration (MIX/TOGGLE/TOUCH/FLASH) **hors payload**.
- `XoneS8MachinePatchBank.hpp` : 24 identifiants MACHINE **virtuels S7** (pas 24 tranches audio AUM), trois slots chacun, **30 paramètres PATCH** : 18 CC + 12 états SPOT 0/1. Douze modes hors payload.
- Les banques sont indépendantes par identifiant et par slot, vides à l'origine ; les valeurs inconnues interdisent un SAVE, jamais de zéro synthétique.
- Le parcours physique de SAVE est SAMPLE : BOT2 puis TOP1..3, ou MACHINE : BOT3 puis TOP1..3 ; une action acceptée ou refusée consomme le TOP et n'entraîne aucun RECALL de secours.
- Un TOP seul peut programmer le rappel S8 au BAR par le pipeline existant. Le rappel écrit uniquement les 28 ou 30 valeurs LIVE admises, puis envoie les adresses virtuelles S7 : SAMPLE CC 0..127 / NOTE 0..95 ; MACHINE CC 128..559 / NOTE 96..383. Les modes S8 et les autres instruments ne sont pas touchés.
- Le wrapper AU réutilise le traitement `samplePatchOnGrid` pour les deux familles ; aucun nouveau moteur d'horloge ni flux audio.
- Les images PATCH S6 à 40/42/84 valeurs sont maintenues **seulement** pour les fixtures des tests archivés, sans migration automatique ni stockage d'un SAVE physique de production.
- La configuration des modes SPOT explicitement renseignée en S8 prend le pas sur les anciens champs S7 LIVE lorsqu'un geste physique SPOT est traité.

## Réserves explicites, sans bloquer l'architecture
- S8 MACHINE 24 est une identification **logique API**, pas une affectation à un plugin, canal ou programme AUM ; choisir ces destinations plus tard.
- Pas de persistance des nouvelles banques S8 dans AU state : relève du bloc 4/5.
- Les alternances S8, BASE et PRESET/RED restent aux blocs 2/5 et 3/5 ; les LED feedback au bloc 4/5.
- Les tests source GCC/Clang et le wrapper macOS/build iOS sont des critères CI ; aucun résultat AUM/iPhone matériel ne peut être déduit de ces tests.
- La branche officielle n'est mise à jour que par fast-forward sur HEAD ayant la CI complète PASS.

Références : `docs/universel/00_CONTRAT_MAITRE_UNIVERSEL_V1.md`, `01_CONTRAT_API_ET_ADAPTATEURS_V1.md`, `02_PLAN_BATAILLE_5_BLOCS_V1.md`, et la décision S8-GEN D01–D09 du 19/10/2026 (Drive).