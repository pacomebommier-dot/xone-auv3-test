# XONE AUv3 — ADDENDUM ACTIF : SH-01A ET MACHINE PAIRES
Décision utilisateur du 4 novembre 2026. Statut : CONTRAT FONCTIONNEL À IMPLÉMENTER / TESTS NON EXÉCUTÉS.
Portée : complément de `00_CONTRAT_MAITRE_UNIVERSEL_V1.md` et `01_CONTRAT_API_ET_ADAPTATEURS_V1.md`, sans modification des archives GOLDEN ni des identités S7. Ces deux demandes ne prouvent aucune fonctionnalité nouvelle déjà présente dans l'AUv3.

## D1 — Intégrer le Roland SH-01A réel à XONE (adaptateur de destination)

**But :** rendre le moteur universel XONE capable de commander et d'observer un SH-01A physique dans AUM, sans remplacer l'API MIDI virtuelle ni hardcoder le Roland dans le moteur.

1. Conserver les ID/CCOUT virtuels XONE (S7 V1.2 MIDI7..14, LED MIDI15). Un adaptateur SH-01A versionné traduit les commandes autorisées vers le port/canal et les CC/PC documentés et vérifiés. Ne jamais copier le remapping de test CH1 dans la production.
2. Raccorder au host AUv3 la vraie entrée MIDI SH -> XONE et la sortie XONE -> SH, avec ports/routes explicites, sans boucle d'écho. La communication vérifiée via sonde Mozaic/AUM ne prouve pas ce raccordement dans l'app.
3. Identifier chaque observation par destination, canal, numéro CC, programme/epoch, valeur, validité et provenance ; mettre à jour LIVE et la référence du pickup. Un changement de programme ouvre une nouvelle epoch et empêche les anciennes valeurs d'être présentées comme les valeurs du programme nouveau. Aucune valeur manquante n'est inventée : UNKNOWN reste UNKNOWN.
4. Distinguer commande envoyée, événement observé et succès musical réel ; ne pas prendre un callback MIDI réussi pour un ACK SH.
5. PATCH/SAVE/LOAD XONE : enregistrer numéro de programme quand connu, valeurs CC connues et masque de validité, selon les propriétaires existants. LOAD = PC ciblé, collecte non bloquante de retours SH, puis application contrôlée des seuls réglages XONE sauvegardés/autorisés. Ne pas exiger exactement 36 CC ni déclarer la mémoire matérielle écrite ; WRITE Roland physique ≠ SAVE XONE.
6. Respecter les valeurs discrètes, les defaults vérifiés, le refus ciblé UNKNOWN, les transitions STOP/NoteOff, MEMORY/PRESET/RED et les exclusions de profils S8. Réutiliser les moteurs existants.
7. Utiliser comme documents de référence : rapport de sonde et contrat du 01/11, handoff Codex V2 ; mapping interactif SH V27 du 04/11 comme **proposition d'interface à consolider**, non comme preuve de binding MIDI ni comme autorisation de toutes ses macros. Les anciens placements de commandes centrales sur MACHINE 2 deviennent caducs selon D2. Ne pas réintroduire automatiquement ces affectations.

**TODO SH-A** : relire profil actuel/host AU ; identifier l'adaptateur et ses entrées/sorties réellement manquantes ; brancher transport et table de paramètres contrôlée.
**TODO SH-B** : intégrer observation PC/CC + epochs/UNKNOWN + LIVE/pickup sans écho.
**TODO SH-C** : raccorder SAVE/LOAD et persistance de valeurs partielles ; conserver les propriétaires XONE.
**TODO SH-D** : CI déterministe, puis recette réelle AUM/iPhone/SH ; documenter résultats et inconnues.

**Tests d'acceptation SH** :
- SH01 : entrée PC/CC canal attendu et autre canal, valeurs 0/127, valeurs inconnues, doublons, ordre inversé et bursts de longueur variable ; bonne provenance et epoch.
- SH02 : changement de programme invalide l'ancienne image LIVE sans convertir UNKNOWN en 0 ; absence de renvoi automatique/feedback loop.
- SH03 : changement physique SH observé -> valeur LIVE mise à jour ; retour sur rotatif XONE -> pickup sans saut, ni émission avant rattrapage.
- SH04 : SAVE XONE puis LOAD -> PC et CC connus seulement, ordre sûr, réponse partielle non bloquante ; pas de faux WRITE ou ACK ; aucune capture/écriture dans d'autres owners.
- SH05 : port/canal absent, reconnect, STOP, replay et défaut inconnu -> refus local sûr, flux virtuel indépendant conservé.
- SH06 : vrai XONE AUv3 dans AUM, MIDI vers SH physique et retours -> tracer les octets/valeurs et observation audible ; verdict matériel distinct de CI. Les sondes Mozaic V11/V12 déjà réalisées ne doivent pas être renommées « test app PASS ».

## D2 — Règle générale de toutes les MACHINE paires : surface FX de l'instrument

**Portée** : pour chaque paire de pages MACHINE (1/2, 3/4, 5/6, 7/8), la page paire (2, 4, 6, 8) sert d'accès rapide aux FX de l'instrument associé. Cette règle porte **uniquement sur les 12 rotatifs centraux (CC) et les 12 SPOT** du Xone K2. Ne pas déduire une association INST physique non documentée : utiliser l'identité source/instrument déterminée par la navigation et les owners existants ; aucune nouvelle destination MIDI imaginée.

1. Les quatre colonnes de trois couples CC/SPOT reflètent **les quatre sous-pages existantes** de la source INST concernée (INST1/INST2 selon la sélection). Elles ne créent ni FX, ni pages, ni paramètres supplémentaires.
2. Vue normale : les 12 CC et 12 SPOT sont des **alias exacts** des trois couples principaux de chacune des quatre sous-pages FX. Vue empruntée : ces mêmes positions accèdent aux trois couples secondaires des tranches empruntées, selon leurs contrats existants.
3. Les 12 SPOT ont exactement les fonctions, modes, gestes, destinations, états, LED et politiques de RESET de leurs SPOT FX d'origine, dans la vue native ou empruntée. **Aucun mode SPOT propre à MACHINE paire** pour cette zone.
4. Un appui court sur une MACHINE paire depuis une autre page l'ouvre en vue native. Un appui LONG sur la même MACHINE paire déjà active bascule native <-> empruntée, sans déclencher l'action courte. Garder la politique standard de capture DOWN/UP et les protections STOP. Aucune navigation réelle dans les quatre sous-pages n'est nécessaire pour adresser leurs valeurs.
5. Les 12 CC/SPOT centraux de la page MACHINE paire **sortent de l'univers MACHINE** : aucune inclusion dans les PATCH, modes ou banques de MACHINE, aucune nouvelle banque de PATCH ou de valeurs, aucun double stockage. Les POTs et FADERs conservent leurs fonctions MACHINE, leurs propriétaires et leur PATCH actuel. Autres commandes non requalifiées par D2 : comportement existant inchangé.
6. Réutiliser les adresses/index logiques, LIVE, mémoire, pickup, LED et mécanismes d'emprunt existants de la source INST. Une modification depuis MACHINE paire doit être visible sur la sous-page INST d'origine et inversement. MEMORY/PRESET/RED opèrent selon leurs domaines existants, sans double capture MACHINE et sans extension implicite.
7. Ne pas créer de deuxième moteur FX, de nouvelle synchronisation, ni de mécanisme de pickup. Vérifier seulement le routage des alias et la non-régression. Les mappings interactifs antérieurs qui plaçaient des paramètres SH indépendants dans les 12 CC/SPOT de MACHINE 2 sont **supplantés pour cette zone seulement** par la présente décision. POTs/FADERs MACHINE restent disponibles à l'adaptateur SH.

**TODO MP-A** : implémenter un routage contextuel d'alias central CC/SPOT pour pages paires 2/4/6/8, source INST résolue, quatre sous-pages et deux vues.
**TODO MP-B** : implémenter/brancher le geste LONG sur page paire déjà active, sans double déclenchement SHORT/LONG.
**TODO MP-C** : enlever la zone centrale paire des domaines PATCH/modes MACHINE tout en préservant POTs/FADERs, leurs banques et leurs exclusions existantes.
**TODO MP-D** : conserver LED/pickup/observations/états, puis tester les huit pages MACHINE, quatre paires et toutes les sorties virtuelles.

**Tests d'acceptation MACHINE paires** :
- MP01 : chaque paire 1/2, 3/4, 5/6, 7/8 : page impaire garde son mapping MACHINE ; page paire route exactement 12 CC/12 SPOT vers la source INST et ses quatre sous-pages.
- MP02 : SHORT page paire depuis ailleurs = vue native ; LONG sur la paire déjà active = vue empruntée ; LONG suivant = native ; aucun double déclenchement, ni changement musical parasite à la seule bascule.
- MP03 : pour les 12 CC et 12 SPOT, comparer adresses S7, commandes, valeurs, SPOT ON/OFF/RESET/TOUCH/FLASH selon contexte et LED avec la sous-page INST correspondante (native puis empruntée).
- MP04 : modifier dans INST -> lire en MACHINE paire -> modifier -> revenir à INST ; identité de valeur, pickup sans saut, sans écho ni double émission. Un seul état logique, pas de « synchronisation » ajoutée.
- MP05 : SAVE/RECALL PATCH MACHINE page paire ne capture/restaure **que** ses POTs/FADERs et autres données MACHINE autorisées, jamais les 12 CC/SPOT alias INST ; mémoire FX native conserve ses propriétaires.
- MP06 : MEMORY/PRESET/RED de la source INST reflètent les modifications faites via MACHINE paire conformément à leur portée existante ; protections UNKNOWN/STOP, changement d'instrument, emprunt et reconnexion préservés.
- MP07 : test dans la vraie AU (monitor MIDI avant route instrument) puis AUM/K2 ; vérifier les deux vues et LED, sans assimiler test source et recette physique.

## Gestion de chantier et statut

- Deux **évolutions nouvelles** D1 et D2, pas deux bugs du moteur universel ; ne pas modifier rétroactivement les preuves CI.
- À intégrer dans le plan de bataille **en chantiers explicites postérieurs ou parallèles à la V0 universelle**, sans modifier silencieusement le compteur historique 6 tâches ni le plan 5 blocs ; leurs points d'entrée techniques sont S8/host/adapter pour D1, NAV/CC/SPOT/owners pour D2. Le statut de chacun reste OUVERT tant que code, tests automatisés et test AUM correspondant n'existent pas.
- Ne modifier ni GOLDEN, ni anciens contrats historiques, ni base de code sans travail de mise en œuvre distinct et preuve de régression.

## Références
- Contrats universels du 28/10 : `docs/universel/00_CONTRAT_MAITRE_UNIVERSEL_V1.md`, `01_CONTRAT_API_ET_ADAPTATEURS_V1.md`.
- Contrat terrain SH du 01/11 : https://drive.google.com/file/d/1u3uMlEadFoqlhBp85DZ_NzvfOpVfttN8/view
- Handoff SH du 01/11 : https://drive.google.com/file/d/1nEfVyFG5L6peG47gkgQ9cb0ktBDT1yiz/view
- Mapping SH V27 du 04/11 (proposition) : https://drive.google.com/file/d/1yeNQSif27FSsfHPGRtK_PSj7ILHHqznG/view
- Matrice owners INST et quatre pages : https://drive.google.com/file/d/1GUt38gN8dV5PalylmMLSMGRRlztTj6my/view
