# Bloc 2 — implémentation native du 28 octobre 2026

## Tâche 1
CUE INST4/8 -> sources INST1/2 (owners PATCH 8/9), LOAD YELLOW4/8 -> YELLOW1/2 (owners 10/11). Chaque source possède trois PATCH de 60 valeurs (deux blocs MACHINE sans les modes S8). Position 4 construit BASE locale sans quatrième slot. Le DOWN capture source et position ; le BAR authentifié applique via S3 et produit les CC/notes nominaux. Les CC CUE/LOAD 931..946 conservent leur confirmation de callback indépendante. Un PATCH vide/incompatible refuse le rappel local sans inventer de contenu.

`saveSourcePatch` capture uniquement des valeurs LIVE connues ; cette API locale est distincte des SAVE physiques SAMPLE/MACHINE existants. Aucun geste SAVE de paire non défini par le contrat n'est inventé. Son adaptateur physique reste à raccorder après qualification ; les tests exécutent réellement la capture et le rappel local, pas un chargement externe.

La file MIDI locale est bornée, réessaie au render suivant sur échec et est annulée par STOP ; les NoteOff déjà dus restent gérés par S4. Aucune destination ou réussite de preset externe n'est déduite.

Preuve locale : test C++ `test_host_source_recall` (12 slots, quatre BASE, autres owners préservés, UNKNOWN/EMPTY, STOP, callback failed/retry). CI Apple/iOS et essais AUM sont distincts.
