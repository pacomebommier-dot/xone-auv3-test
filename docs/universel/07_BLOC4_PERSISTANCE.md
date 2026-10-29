# Bloc 4 — persistance native — 29 octobre 2026

## Raccordement réalisé
Le document AU `XONE_DOCUMENT_V2` contient LIVE, NAV aplati, états appliqués MUTE/SOLO, choix SOFT sans gates historiques, engagements MEMORY/PRESET, sélections CUE/LOAD, banques PATCH S8 SAMPLE/MACHINE et paires locales, MEMORY des 28 pages, PRESET1..4, référence INIT explicite, profils BASE/modes et bindings des alternatives. Les banques historiques S6 explicitement alimentées sont conservées sans import automatique. Les PATCH vides restent vides ; UNKNOWN reste UNKNOWN.

Les champs sont encodés explicitement, indépendamment du padding ou de la disposition C++. Version2, taille bornée et CRC32 ; contrôles typés avant publication. Une image invalide n'écrase pas l'état ni les banques. Les révisions de slots sont conservées ; CoreV3 maintient les bases de révision importées devant le stockage S6 gelé, sans modifier celui-ci. Le SAVE suivant incrémente la révision conservée.

Le host reçoit un snapshot cohérent via le même échange SPSC que LIVE. Une demande de restauration prend effet à une frontière de rendu, après les vrais UP et la fermeture des NoteOff dus. Le dernier restore accepté gagne entre V1 et V2. Aucun verrou, sérialiseur de fichier ou allocation n'est ajouté dans le rendu ; les tampons de documents sont alloués à la construction du moteur.

Les sélections CUE/LOAD restaurées sont des métadonnées désirées, avec confirmation d’émission inconnue : aucun CC LOAD/CUE historique n’est rejoué. Une nouvelle demande physique conserve sa qualification et son BAR.

Les anciens documents `XONE_LAST_LIVE_V1` restent lisibles avec leur comportement LIVE existant. V2 est prioritaire lorsqu'il est présent ; un V2 invalide n'autorise pas un repli silencieux vers une copie V1. La clé V1 reste publiée pour compatibilité de lecture.

## Reprise des sorties
V2 reconstruit uniquement les CC/notes nominaux connus, dans les adresses virtuelles S7. LOOP m reste interne et les trois gains effectifs sont recalculés. Les modes S8 ne deviennent pas des notes. Les états au repos des SPOT momentanés capturés remplacent leurs fronts temporaires ; aucun replay TOP/GROUP/LONG, SOFT historique, DJ ou geste physique. Les demandes BAR/BEAT, RED actif, retours LONG, observations et caches sent ne sont pas restaurés. Les callbacks restent la seule confirmation d'émission ; échec puis reprise au buffer suivant.

Les16 états MUTE appliqués sont reprojetés avec cache envoyé invalide, y compris les OFF ; aucune absence de mute ne vaut preuve d’émission. Le cache LED existant est invalidé pour restituer la vue courante au reopen, avec purge OFF avant ON et confirmation réelle du callback. Ce lot ne clôt pas les vues complémentaires des 34 positions : MEMORY/PATCH/PRESET/RED et leurs détails restent à raccorder aux règles S7 dans la suite du Bloc4.

## Preuves
Suite dédiée : roundtrip PATCH/MEMORY/PRESET/configuration, révision MEMORY2 puis SAVE3, pages cachées SAMPLE/MACHINE, taille bornée, CRC, schéma futur, bool invalide avec CRC réparé, refus typé sans mutation, propriétaire tenu au restore, priorité V1/V2 et retry MIDI. Le banc du véritable wrapper AU vérifie V2 via fullState/fullStateForDocument et la projection nominale au reopen. Les résultats Apple/iOS sont ceux de la CI du commit ; aucune recette AUM/iPhone ou réception instrumentale n'est déduite.
