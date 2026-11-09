# XONE AUv3 — DÉCISION UTILISATEUR DU 09/11/2026
## SOC-06 — Unification des gestes SAVE / RECALL (complément à SOC-01..05)

**Statut : décision NEW validée ; implémentation SOC-06 publiée dans la PR #24 ; CI complète, build iOS et recette physique AUM encore à confirmer avant IPA.**
**Autorité :** présente décision, postérieure aux arbitrages des 02/10 et 05/11, prévaut **uniquement pour la gestuelle SAVE/RECALL des emplacements utilisateur**. Elle ne remplace ni le GOLDEN (provenance), ni les règles musicales de rappel, ni les exceptions SH-01A, ni les règles des gestes de navigation hors de ce conflit.

### Règle transversale, toujours décidée au VRAI NoteOff / relâchement
Pour tout slot de mémoire utilisateur SAVE/RECALL concerné :
- **Slot VIDE + appui COURT (<200 ms)** : première capture **SAVE**, sans RECALL fictif.
- **Slot VIDE + appui LONG (>=200 ms)** : première capture **SAVE**, une seule fois.
- **Slot RENSEIGNÉ + appui COURT (<200 ms)** : **RECALL** (au BEAT/BAR/immédiat selon le contrat de cette mémoire).
- **Slot RENSEIGNÉ + appui LONG (>=200 ms)** : **SAVE / réécriture** avec les valeurs LIVE du propriétaire capturé au DOWN. **Jamais de RECALL avant SAVE.**
- Un SAVE refusé (UNKNOWN réel non admissible, conflit de propriétaire, profil interdit, document corrompu, etc.) n'est pas converti en RECALL de secours, et n'affiche aucun faux succès.
- Un geste a **un propriétaire**, une cible et un contexte capturés au DOWN ; un seul effet musical au UP ; aucun changement de cible entre-temps ; STOP annule un geste en vol sans action rétroactive ni UP synthétique. Le seuil 200 ms est celui existant (égalité = LONG).
- Le contenu et le statut EMPTY/CURRENT/ALTERED/PENDING, la portée par owner, le pickup, les dettes NoteOff, les LED et la persistance restent régis par leurs contrats spécialisés.

### Matrice normative à appliquer à l'APP
| Commande physique | Slot vide : COURT ou LONG | Slot rempli : COURT | Slot rempli : LONG | Remarques |
|---|---|---|---|---|
| MEMORY INST/YELLOW, slots locaux | SAVE initial | RECALL au NEXT_BEAT | SAVE / remplacer | **Codé SOC-06 :** vrai UP court/long, protection de l'indexation des propriétaires INST/YELLOW ; tests natifs spécifiques PASS. |
| MEMORY DRUMS/FILL, individuel ou groupé (1–3) | SAVE initial, y compris groupé | RECALL au NEXT_BEAT | SAVE / remplacer sur la portée son/groupe | Déjà spécifié 05/11 ; préserver le LIVE d'ouverture des membres vides pour rappel groupé et le statut vide. |
| PATCH INST/YELLOW via CUE/LOAD1–3 | SAVE initial | RECALL au NEXT_BAR | SAVE / remplacer | Règle 05/11 conservée ; application au vrai relâchement. |
| BASE local via CUE4 | N/A : référence BASE existante / initialisée | RECALL BASE | SAVE BASE complet | Ne crée pas PATCH4 ; destination et FX capturés, SH spécial. |
| PATCH SAMPLE via TOP1–3 | SAVE initial | RECALL PATCH | SAVE / remplacer PATCH | **NOUVEAU :** TOP seul décide COURT/LONG. Supprime la nécessité BOT2 tenu + TOP pour SAVE. |
| PATCH MACHINE via TOP1–3 | SAVE initial | RECALL PATCH | SAVE / remplacer PATCH | **NOUVEAU :** TOP seul décide COURT/LONG. Supprime la nécessité BOT3 tenu + TOP pour SAVE. |
| PRESET global, slots1–4 | SAVE initial | RECALL au NEXT_BAR | SAVE / remplacer snapshot global | **Codé SOC-06 :** qualification au vrai UP, RED inchangé ; tests natifs spécifiques PASS. |

### Suppression de l'ancien conflit BOT / TOP
- Les anciennes commandes **SAMPLE : BOT2 maintenu + TOP1–3 = SAVE** et **MACHINE : BOT3 maintenu + TOP1–3 = SAVE** sont **supplantées** dans NEW par **TOP1–3 LONG = SAVE**, **TOP1–3 COURT = RECALL** (ou SAVE initial si vide).
- BOT2 ne doit **plus être capturé par un « candidat SAVE » en SAMPLE** ; son SMART normal reprend (court = MUTE persistant selon navigation ; long = MUTE momentané puis retour au contexte d'origine). BOT3 reprend de même son SMART GLOBAL normal en MACHINE.
- Ne pas déclencher SAVE sur TOP DOWN en anticipant un LONG, ni déclencher RECALL au DOWN. Qualifier au vrai relâchement, avec contexte/cible figés. Un BOT pressé simultanément ne transforme pas le TOP en SAVE : une seule interprétation/owner.
- Les combos de navigation BOT2→BOT4 MACHINE, BOT2→BOT3 SAMPLE, BOT3→BOT2 LOOP, BOT3→BOT4 RESET, BOT3→BOT1 PRESET, BOT2→BOT1 retour SOCLE et le BOT4+SPOT de modes restent indépendants et inchangés.
- TOP4 n'est **jamais PATCH4** ; il garde FILTER GLOBAL / RESET selon son contrat.

### Exception obligatoire SH-01A : Roland est maître
- Programmes SH-01A : SAVE sonore **uniquement par WRITE physique du Roland** ; **aucun SAVE XONE** sur PATCH1–3, que le geste soit long, court, BOT+TOP, CUE/LOAD, etc.
- XONE commande les rappels SH par **PC seul** ; les trois miroirs PATCH sont actualisés **automatiquement sur réception complète des 36 CC** ; jamais d'écriture MIDI fictive ou de succès SAVE.
- Le **CUE4 LONG** reste autorisé à sauver la référence BASE **et les FX locaux XONE**, pas le programme sonore du SH. Le programme BASE SH est rappelé via PC0.
- Les profils non-SH gardent leur stockage et leurs sorties MIDI universelles sans supposer un instrument externe disponible.

### Ce que cette règle NE change PAS
- RED/BOT1 est un snapshot temporaire, **pas un slot utilisateur à sauvegarder par long** ; sa grammaire SMART et son restore restent inchangés.
- MEMORY rappelle au BEAT ; PATCH/CUE/LOAD/PRESET au BAR selon leurs contrats. Le remplacement SAVE ne décale pas une opération musicalement autrement définie.
- Les MEMORY/MUTE/SOLO/FX, les alias de MACHINE paires, les modes SPOT S8 exclus des PATCH MACHINE, les filtres et les mécanismes de STOP conservent leurs portées existantes.
- GOLDEN est rapporté comme historique ; si un geste NEW n'existait pas dans GOLDEN, écrire **S/O** dans les matrices, pas « N/A », ni une fonction inventée.

### Recette SOC-06 — critères bloquants avant IPA
1. Pour chacun des types de slot ci-dessus, **vide court**, **vide long**, **rempli court**, **rempli long**, égalité exactement **200 ms**, et annulation STOP ; vérifier une seule opération et le bon propriétaire.
2. SAMPLE/MACHINE TOP1..3 LONG sans BOT = SAVE ; TOP court = RECALL ; BOT2/BOT3 LONG sans TOP = **momentané** et restauration fidèle. Aucun SAVE BOT+TOP résiduel ni RECALL au TOP DOWN.
3. PRESET snapshot long = remplacement atomique ; short rempli = rappel BAR ; EMPTY court = première capture.
4. SH : LONG TOP/CUE PATCH ne sauvegarde aucun programme ; seul WRITE physique SH fait foi ; miroirs 36/36 ; CUE4 FX XONE préservés.
5. NAV concurrente, DJ browser/PUSH et SPOT+BOT4, changement de page pendant maintien, échec LIVE UNKNOWN, sortie MIDI, LED, sauvegarde/réouverture AUM, NoteOff/STOP.
6. Tests natifs + sanitizers + wrapper AU + build iOS puis recette physique K2/AUM. **Aucun PASS IPA automatique sur simple lecture du contrat.**

### Traçabilité / autorités antérieures
- [Arbitrage des gestes locaux 02/10 — HISTORIQUE pour SAVE BOT+TOP](https://drive.google.com/file/d/1WRiaGnGkLm_MjsN8zJ1f45PB072gcVH_/view)
- [Addendum du 05/11 — conserver CUE4, DRUMS/FILL, SH et règle slots vides](11_ADDENDUM_BASE_CUE4_DRUMS_FILL_20261105.md)
- [Contrat SH miroir36](05_CONTRAT_SH01A_MIROIR_PATCH36_V1_20261104.md)
- [Moteur APP](../../Shared/XoneHostEngine.hpp)
- [PR pré-IPA SOC-01..05, qui accueille ce SOC-06](https://github.com/pacomebommier-dot/xone-auv3-test/pull/24)

**Suivi de réalisation au 09/11/2026 :** les raccourcis SAVE BOT2+TOP/BOT3+TOP ont été supprimés du host ; SAMPLE/MACHINE TOP1–3 qualifient au vrai UP ; BOT2 LONG de SAMPLE ouvre MUTE momentanément puis revient à l'état d'origine ; BOT3 LONG de MACHINE ouvre GLOBAL momentanément puis revient à MACHINE. MEMORY INST/YELLOW et PRESET distinguent désormais SAVE long et RECALL court. Les programmes SH-01A restent exclusivement sous WRITE physique.

La correction de sécurité `1e90683` empêche les propriétaires INST/YELLOW d'indexer les files rhythm hors plage. Les superpositions PRESET/RESET ne déclenchent pas un SAVE SAMPLE parasite (`dc6c15b`). Les tests SOC-06 SAMPLE, MACHINE, MEMORY, PRESET et le banc Apple ont passé dans les CI intermédiaires. **La CI complète du HEAD, les sanitizers, le build iOS et la recette physique AUM restent à clôturer.** Ce statut n'est pas une déclaration d'IPA signée/prête.
