# Bloc 2 â€” implÃ©mentation native du 28 octobre 2026

## TÃ¢che 1
CUE INST4/8 -> sources INST1/2 (owners PATCH 8/9), LOAD YELLOW4/8 -> YELLOW1/2 (owners 10/11). Chaque source possÃ¨de trois PATCH de 60 valeurs (deux blocs MACHINE sans les modes S8). Position 4 construit BASE locale sans quatriÃ¨me slot. Le DOWN capture source et position ; le BAR authentifiÃ© applique via S3 et produit les CC/notes nominaux. Les CC CUE/LOAD 931..946 conservent leur confirmation de callback indÃ©pendante. Un PATCH vide/incompatible refuse le rappel local sans inventer de contenu.

`saveSourcePatch` capture uniquement des valeurs LIVE connues ; cette API locale est distincte des SAVE physiques SAMPLE/MACHINE existants. Aucun geste SAVE de paire non dÃ©fini par le contrat n'est inventÃ©. Son adaptateur physique reste Ã  raccorder aprÃ¨s qualification ; les tests exÃ©cutent rÃ©ellement la capture et le rappel local, pas un chargement externe.

La file MIDI locale est bornÃ©e, rÃ©essaie au render suivant sur Ã©chec et est annulÃ©e par STOP ; les NoteOff dÃ©jÃ  dus restent gÃ©rÃ©s par S4. Aucune destination ou rÃ©ussite de preset externe n'est dÃ©duite.

Preuve locale : test C++ `test_host_source_recall` (12 slots, quatre BASE, autres owners prÃ©servÃ©s, UNKNOWN/EMPTY, STOP, callback failed/retry). CI Apple/iOS et essais AUM sont distincts.

## Tâche 2
Chaque PATCH/BASE compose maintenant 97 champs : les 60 valeurs/états MACHINE sans modes S8 et les 37 cibles SOCLE locales explicitement autorisées (24 CC, 12 SPOT, filtre source). Le PATCH restaure son payload et remet uniquement son SOCLE à zéro. BASE utilise les seuls zéros provisoires autorisés. Les modes S8, MIX1328..1331, isolations et INIT partagés restent intacts. Les pickup CC réellement affectés sont réarmés même si leur ancienne valeur physique aurait permis de poursuivre immédiatement.

La mutation LIVE est validée entièrement dans S3 avant publication MIDI. La file refuse un lot qui dépasserait sa capacité ; aucun succès externe BASE n'est annoncé. Les fins de lignes ont été rétablies pour conserver une différence finale limitée.
