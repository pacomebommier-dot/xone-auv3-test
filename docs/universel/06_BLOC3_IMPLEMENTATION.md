# Bloc 3 — snapshots globaux natifs — 28 octobre 2026

## Réalisé
Quatre banques PRESET natives, initialement vides. Sur la face PRESET qualifiée, les quatre boutons36..39 capturent au premier engagement puis demandent le rappel au BAR. INIT32 reste une commande distincte au BEAT, sans cinquième banque ; sa référence locale doit être explicitement renseignée, sinon refus sans RESET inventé. `showPresetPage` et `leavePresetPage` sont des commandes natives de face ; aucun nouveau combo K2 n'est inventé pour remplacer BOT2/MUTE.

RED physique BOT1/note12 : premier DOWN capture le monde courant ; premier UP court conserve RED actif ; second appui court ou UP long restaure immédiatement. STOP retire son autorité sans restauration et son ancien UP reste inerte. SAVE PATCH est refusé pendant RED, sans rappel de secours.

Les images musicales contiennent valeurs connues ou UNKNOWN, NAV aplati, états appliqués MUTE/SOLO, SOFT, sélections CUE/LOAD désirées et engagements MEMORY/PRESET. Elles ne contiennent ni gestes, ni retours LONG, ni pending, ni caches d'émission, ni observations, ni banques utilisateur. Les modes S8 SAMPLE/MACHINE sont exclus. Une restauration conserve leur configuration actuelle.

S3 prépare et publie LIVE/NAV ensemble. L'archive LONG demeure dans son autorité existante : un PRESET peut publier GLOBAL pendant LONG SAMPLE, puis le vrai UP retrouve le contexte antérieur sans modifier les valeurs rappelées (Q11). Aucun service DJ/track n'est déclenché par le rappel (Q10). Les témoins valid PRESET suivent leur cycle vivant ; aucun valid historique n'est recopié. Q12 compare les19 composantes natives sur causes physiques note-equivalent et qualifications NAV ; CC/fader/filter seuls, RED direct et applications internes ne l'invalident pas.

La restauration RED annule tous les MUTE en attente, y compris ceux précédant sa capture. Les autres demandes musicales vivantes gardent leur propre échéance. Les snapshots ne ressuscitent aucun pending. INIT et PRESET utilisent l'ordonnanceur déjà présent, avec BEAT/BAR et ordre d'admission distincts ; une préparation temporairement indisponible conserve son ordre.

## Sortie et sûreté
Les valeurs connues produisent leurs CC/notes S7 ordinaires. Un UNKNOWN reste UNKNOWN et n'émet aucune valeur de remplacement. Le facteur LOOP m reste interne ; seuls les trois gains effectifs sont émis. Aucun opcode PRESET ni service externe supposé réussi.

Un rappel global remplace la projection absolue obsolète, même si sa file est pleine, et conserve les demandes musicales futures. Avant les nouvelles activations, il conserve et ferme les NoteOff dus, notamment ceux des TOUCH capturés. Un échec de callback bloque les messages suivants jusqu'au render suivant ; STOP conserve les dettes S4. Le contrôle RED n'a pas besoin d'une place dans la file d'ordonnancement.

La capacité du scheduler couvre les28 pages MEMORY, quatre sources, PRESET, INIT et le domaine d'alternatives. La file nominale et le staging global sont bornés à2048 messages pour les CC/états et leurs OFF de fermeture. Ce n'est pas un transport MIDI privé ni un ACK matériel.

## Preuves et reste
64 suites C++ locales et trois contrôles d'intégrité PASS au premier raccordement. Tests dédiés : quatre slots physiques, RED court/long/STOP, UNKNOWN, modes S8 exclus, Q11, Q12, RED/MUTE des deux côtés de la capture, conservation d'une demande source, ordre INIT/PRESET et projection saturée. Le banc AU macOS ajoute RED physique et l'échec callback/reprise ; son résultat doit être lu dans la CI Apple.

Les alternatives S8, leurs bindings/configurations propres et leur fermeture OFF avant bascule sont la suite de ce même Bloc3. Leur arbitre natif existant n'est pas annoncé raccordé par ce seul commit. Accès UI complet de la face PRESET, feedback et persistance étendue restent à terminer ; aucune recette AUM/iPhone n'est déduite de ces tests.
