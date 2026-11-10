# XONE V0.2 — checkpoint pré-IPA SOC-01..06 — 10 novembre 2026

## Périmètre figé et décisions
PR de travail : https://github.com/pacomebommier-dot/xone-auv3-test/pull/24 ; branche `prep/ipa-soc01-05-recette-20261109`. PR **draft**, pas de fusion, pas de signature Apple, aucune recette matérielle déclarée PASS.

La règle utilisateur SOC-06 figure dans `docs/universel/12_DECISION_UNIFICATION_GESTES_SAVE_RECALL_20261109.md` :
- slot VIDE : court et long = SAVE initial ;
- slot RENSEIGNÉ : court = RECALL ; long **>=200ms** = SAVE/remplacement ;
- vrai UP qualifie le geste, propriétaire/slot capturés au DOWN ; STOP et UNKNOWN ne déclenchent aucun effet rétroactif ni faux succès ;
- SAMPLE : TOP1..3 décide SAVE/RECALL sans BOT2 ; BOT2 LONG devient MUTE momentané, retour SAMPLE au vrai UP ;
- MACHINE : TOP1..3 décide SAVE/RECALL sans BOT3 ; BOT3 LONG devient GLOBAL momentané, retour MACHINE au vrai UP ;
- les anciens accords BOT2+TOP/BOT3+TOP **ne commandent plus SAVE** ;
- MEMORY INST/YELLOW/DRUMS/FILL, CUE/LOAD, BASE CUE4 et PRESET conservent leurs quantifications BEAT/BAR et leurs portées spécialisées ; RED reste distinct ;
- SH-01A : sauvegarde du programme **WRITE physique uniquement** ; aucun LONG XONE ne sauve/émet de WRITE ni ne transforme LONG en RECALL PC. Les FX XONE BASE restent mémorisables par CUE4 LONG.

### Audit SOC-06 réalisé dans le dépôt
- Corrigé : sous-débordement d'index MEMORY INST/YELLOW vers les tableaux rhythm (`1e90683`).
- Corrigé : impossibilité de sauver un PATCH SAMPLE depuis les superpositions PRESET/RESET (`dc6c15b`).
- Adapté : ancien parcours universel BOT3+TOP vers TOP seul pour SAVE slot MACHINE vide (`dcd9d2a`).
- Les 12 headers historiques protégés restent intacts, contrôlés par `tools/verify_source.py`.

## Première chaîne iOS totalement verte — preuve historique immuable
- Run CI : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/42668972726
- Branche de PR à ce moment : `dcd9d2af010f2ead7e4b71fac3b51a8dc2f6dba0`.
- `COMMIT.txt` de l'artefact : `46fac3a1018aa8fadb7f1b3587abfb1e25810b3b`, **merge de test PR** entre base `e0587ead...` et tête `dcd9d2a...` ; cette différence de SHA est NORMALE.
- Artefact : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/42668972726/artifacts/13699880575
- SHA256 contrôlé des octets IPA : `b265f58703384fd4f54c49d0dbb9848a564501e519495b3e3ad8c06715fbe7bc`, conforme à `IPA_SHA256.txt`.
- `XoneEngineNative_V0_2_UNSIGNED.ipa` contient `XoneEngineTest.app` et `XoneMidiAU.appex`, versions `0.2 (2)`, IDs `org.xonelab.XoneEngineNativeV02` et `org.xonelab.XoneEngineNativeV02.XoneMidiAU`, type `aumi`, fabricant `XnLb`, subtype `Xn02`. **Aucune signature n'est présente.**
- Résultat du run : PASS `g++`, PASS `clang++`, PASS banc Apple, PASS suites macOS, PASS ASan/UBSan, PASS `xcodegen` et build unsigned iOS. Ceci prouve **la construction logicielle**, pas l'émission MIDI vers le K2 sur iPhone/AUM.

**IMPORTANT :** la recette AUM embarquée dans l'artefact de ce premier run contient encore l'ancien geste MACHINE BOT3+TOP. Les documents ont été corrigés ensuite dans cette branche. **Ne pas suivre l'ancien E04 de cette archive.** Pour le binaire historique, suivre le protocole courant `RECETTE_AUM_V02.md` : **TOP seul court sur un slot vide = SAVE initial**. Les builds CI postérieurs aux commits documentaires incluront le nouveau protocole. Comparer leur `COMMIT.txt` et leurs propres hash IPA avant d'installer ou signer.

## Documents et matrice de recette
- `docs/reprise/RECETTE_AUM_V02.md` : E04 corrigé ; scénarios E18..E25 SOC-06 ajoutés.
- `docs/reprise/RESULTATS_AUM_V0.csv` : E00..E25 tous initialisés à `NON_TESTE`.
- `docs/reprise/RECETTE_SH36_MACHINE_20261104.md` : T11 couvre explicitement l'exception WRITE SH.
- Ajouter PRE/POST via les sondes distinctes ; MUSIC CH7–14, LED CH15 ; K2 OU K2_SIM, jamais les deux ; arrêter et observer les OFF avant suppression de la tranche.

### Reste BLOQUANT avant IPA pour recette physique
1. Choisir l'artefact du **dernier run CI SUCCESS** après les dernières modifications documentaires, conserver run/COMMIT.txt/IPA_SHA256.txt et identifier la tête PR vs merge SHA.
2. **Signer app et extension pour le vrai device avec l'équipe Apple appropriée**, depuis les sources du même SHA ; le binaire non signé du ZIP CI ne s'installe pas directement sur iPhone.
3. Installer et confirmer apparition du `aumi/XnLb/Xn02` dans AUM, préparer routes PRE/POST, transport, canaux, K2 physical feedback CH15. Aucun de ces résultats n'est présumé.
4. Exécuter **E00..E25**, compléter le CSV sans extrapoler. Une fixture chargée explicitement ne vaut pas preuve « parcours sans fixture ».
5. SH réel : exécuter T01..T11, incluant WRITE physique, PC seul et miroir36/36 ; mesure CPU/xruns sur appareil et retrait AU après NOTE-OFF.
6. Garder PR draft tant que la preuve de signature/installation/routage/LED/NOTE-OFF manquante n'est pas obtenue.

## Règle d'annonce
**PASS natif/CI ≠ PASS AUM/K2 ≠ IPA signée.** Toute reprise doit annoncer ces trois statuts séparément. Ne pas relancer un audit global inutile ; reprendre les seuls scénarios non confirmés, avec commit/source exact.
