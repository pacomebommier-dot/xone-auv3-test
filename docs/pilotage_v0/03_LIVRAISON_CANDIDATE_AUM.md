# XONE AUv3 V0 — CANDIDATE AUM
Livraison logicielle du28 octobre2026 ; recette appareil NON_TESTE_AUM.

## Identification exacte
- Branche : `feature/native-e3-bar-beat-reprise` ; **SHA binaire candidat : `2cedd1eefe74db4fef4ea44764164391d3118911`**. Les éventuels commits documentaires suivants ne reconstruisent pas ce binaire.
- [CI complète PASS40671623461](https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40671623461) : GCC, Clang, intégrité historique, vraie classe AU macOS, Apple Clang, ASan/UBSan et build iOS.
- [Paquet iOS et recette](https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40671623461/artifacts/12808951685), artefact12808951685, conservé par GitHub jusqu’au27 novembre2026. Copie locale : `output/delivery/XONE_V0_CANDIDATE_AUM_2cedd1e.zip` dans le checkout candidate.
- ZIP livré SHA256 : `940a05636c6dd450dded7bb6d74b95ae9187a4e9b66f0b5079304025ea32bad1`.
- IPA non signé : `XoneEngineNative_V0_2_UNSIGNED.ipa`, SHA256 `a53c7fea9b46b4d8faf7a09c3019951d90cb4fe43f192d0fc8523834b8866653`.
- Archive téléchargée et vérifiée : COMMIT.txt exact, CRC ZIP/IPA valides, app et extension arm64 embarquées, composant aumi/XnLb/Xn02, identifiants app/extension cohérents. Sources wrapper/moteur/fixture du ZIP comparées aux blobs du SHA candidat.

## Installation et première recette
Suivre [RECETTE_AUM_V02.md](../reprise/RECETTE_AUM_V02.md) : signature Xcode de l’app **et** de l’extension, installation sur son appareil, session isolée K2 ou Mozaic→XONE→moniteur. Le paquet contient les sources exactes XONE_V0_SOURCES.zip et le simulateur Mozaic existant. Les identifiants visibles restent V0.2 pour préserver la continuité d’installation.

Le bouton de recette charge explicitement l’image simulée déjà utilisée par le harnais, via la restauration AU existante. Il ne définit aucun preset ou profil réel. Au démarrage ordinaire, UNKNOWN et refus locaux restent conservés. La session .aum doit être créée et sauvegardée dans AUM selon la fiche ; aucun fichier hôte ni résultat matériel n’a été fabriqué.

## Portée prouvée et frontières
| État | Portée |
|---|---|
| OPERATIONNEL — preuve logicielle | Vrai wrapper AU : K2/S1/S2/NAV/S3/S4→callback MIDI, CC/pickup, SPOT ON/OFF, STOP et callbacks échoués, MUTE/LED sur BAR, document et nouvelle instance ; sélection/LOOP et SOLO couverts par les suites moteur. Réallocation vérifiée sans callback puis avec sortie reconnectée. |
| A_CONFIGURER | Instruments, presets, sources audio, profils/ports, MIDI Learn AUM, PATCH/BASE/RESET/FILTER absents. UNKNOWN/INVALID restent explicites. |
| DIFFERE | MACHINE local préservé hors candidate ; finalisation PATCH/MEMORY/CUE/LOAD/BASE, PRESET/RED, LED secondaires, persistance étendue, exécuteurs DJ LOOP/Browser LOAD. Les six tâches restent ouvertes et distinctes de V0. |
| NON_TESTE_AUM | Signature/installation réelles, interface sur appareil, sept essais de la fiche, compilation Mozaic native, session .aum, K2 et destinations matérielles. |

STOP et vrais UP, puis observation des OFF, **avant retrait définitif d’une instance**. Aucun callback hors rendu n’est supposé : si l’hôte efface sa sortie à la désallocation, la dette est conservée et réémise au premier rendu avec callback reconnecté. Une destruction sans STOP ni nouveau rendu ne constitue pas une livraison externe d’OFF garantie.

## Préservation et reprise
Le checkout `XONE_AUV3_REPRISE_20261027` conserve son HEAD09b30ca et son travail non publié. Sauvegarde `SAUVEGARDES/V0_AVANT_PRODUCTION_20261028` : patch suivi identique et438 fichiers non suivis comparés par SHA256 ; VERIFICATION.json joint. Le travail V0 est dans `XONE_AUV3_V0_CANDIDATE`. Aucun merge main ; aucune source musicale gelée modifiée.

Prochaine action : signer/installer la candidate exacte, effectuer la fiche AUM, relever PASS/FAIL/NON TESTABLE avec SHA et captures MIDI. La phase de recette réelle commence ; aucun PASS iPhone/AUM n’est déclaré ici.
