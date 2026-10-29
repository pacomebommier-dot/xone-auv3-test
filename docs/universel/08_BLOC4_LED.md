# Bloc 4 — vues LED natives — 29 octobre 2026

## Raccordé
Un seul cache physique couvre les34 positions S7 : BASE16, SPOT12, TOP4, BOT1 et BOT3. Les bindings restent ceux de NativeSlices/L10 : MIDI15 uniquement, BASE row-major, rouge puis +36 ambre et +72 vert ; BOT1 uniquement note12 rouge, BOT3 notes15/19/23. UNKNOWN est conservé dans la vue et rendu sombre par purge des bindings prouvés, sans fabriquer un état musical OFF.

La projection lit les autorités existantes : NAV/S3, LIVE, banques S6/S8, engagements MEMORY/PRESET, CUE/LOAD et alternatives. Aucun état canonique musical ni ordonnanceur supplémentaire. PATCH vide vert, courant rouge, altéré ambre ; comparaison des28 SAMPLE,30 MACHINE ou60 de la paire. Les modes S8 ne contaminent pas cette comparaison. Configuration incompatible ou comparaison ne pouvant établir CURRENT/ALTERED : UNKNOWN.

MEMORY engagée rouge ; attente BEAT verte clignotante. Les28 propriétaires sont qualifiés par les routes visibles ; les banques cachées ne sont pas modifiées. La colonne SOFT conserve sa sélection logique et son attente BAR vertes. CUE/LOAD sélection émise verte, demande en attente ambre clignotante : un callback en échec ne donne pas une sélection confirmée. Les métadonnées seules d'un document n'inventent aucun succès d'émission CUE/LOAD ou preset instrumental.

PRESET vide vert, engagé ambre, validité native Q12 rouge ; attente BAR ambre clignotante. INIT reste distinct, vert avec attente BEAT. RED n'allume que BOT1 rouge pendant son autorité vivante. Les groupes consultés et emplacements S8 suivent L10 : groupe rouge ; instrument ACTIVE propre vert, ACTIVE du pair ambre, PENDING propre vert clignotant, PENDING du pair ambre. Un binding absent reste UNKNOWN, sans destination inventée.

Les états SPOT utilisent la palette S7 des modes et familles ; le mode S8 de l'instrument actif prévaut. DJ relatif montre le centre63 éteint uniquement si la valeur est connue. Les autres identités sans couleur stable contractuelle restent UNKNOWN.

## Sortie et cycle de vie
OFF de l'ancienne couleur avant ON de la nouvelle. Seul le callback réussi met à jour le cache envoyé. Echec : les bindings de cette position sont inconnus, les autres positions continuent, et la purge est réessayée au buffer suivant. Une position en échec n'est pas réessayée dans le même buffer. Le budget AU de136 paquets couvre les100 bindings OFF prouvés et au plus34 ON ; aucune note inexistante n'est émise pour BOT1.

Blink à phases100ms. Le feedback pickup CC4..15 utilise les positions16..27 et les palettes de mode connues pendant100ms ; il peut précéder l'acquisition sans commit LIVE inventé. Si cette même couleur est déjà prouvée envoyée, le flash est un OFF temporaire puis le retour de la vue, comme Golden ; un cache incertain ne permet pas de supposer cette couleur déjà allumée. UNKNOWN/refus ne rejoue pas un ancien flash. Changement d'image/NAV et STOP annulent les flashes. START et réouverture invalident le cache pour restituer la vue appliquée, sans replay musical.

Le paramètre AU `Refresh controller LEDs` (adresse AU1, distincte de MIDI) permet une resynchronisation explicite après reconnexion. Intention UI atomique consommée au rendu ; automation admise au sample fourni. Sa lecture revient à0 ; aucune modification de LIVE, des banques, du transport ou du scheduler. Aucun succès matériel n'est déduit du callback MIDI.

## Preuves et portée
67 suites natives locales et trois contrôles d'intégrité PASS avant publication, puis tests LED ciblés rejoués après les gardes de comparaison et de CUE/LOAD. Suite nouvelle :34 bindings,100 OFF/34 ON, UNKNOWN, blink, flash/expiration, retry, PRESET/RED, PATCH CURRENT/ALTERED/UNKNOWN,28 propriétaires MEMORY, alternatives, pickup avant acquisition et resynchronisation. Le banc AU macOS vérifie le premier feedback hors MUTE, ses transitions et les deux chemins du paramètre de resynchronisation.

Les anciennes assertions64 paquets à la première entrée MUTE décrivaient le cache froid avant sa seule vue initiale. Elles sont remplacées par une preuve exacte :100 OFF + sélection avant MUTE, puis17 transitions (ancienne couleur OFF et16 verts ON), échec et reprise vérifiés octet par octet. Le banc BASE-only historique du cache conserve ses64 paquets et ses assertions.

Références ciblées : CONTRAT_LED_7_2_V1 du15/10 ; L10_S7_NONCURRENT @LedContext/@x4/@x5/@x6/@x7 et @LedInitBindings ; décisions S8 sur les modes et alternatives. La clôture logicielle exige la CI complète du HEAD ; aucune validation K2 physique ou AUM/iPhone n'est déduite.
