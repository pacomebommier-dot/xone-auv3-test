# XONE AUv3 — CONTRAT SH-01A : TROIS PATCH MIROIRS DE 36 CC
**Décision utilisateur : 4 novembre 2026. Statut : NORMATIF PRODUIT / CODE ET RECETTE À RÉALISER.**

## 0. Portée et priorité
Ce contrat décrit **exclusivement le profil matériel Roland SH-01A** et remplace, sur ce seul profil, toute disposition antérieure prévoyant un SAVE XONE autonome, une validation manuelle XONE, un rappel XONE qui réémet les CC d'un PATCH, ou un PATCH SH couplé aux FX. Il précise D1 de `04_ADDENDUM_SH01A_MACHINES_PAIRES_2026-11-04.md`. La règle D2 sur les pages MACHINE paires reste intacte.

Pour tout autre synthé/plugin, les contrats génériques XONE PATCH/MEMORY/CUE/LOAD/BASE/S7/S8 demeurent inchangés. Les contrats SH du 1er novembre et le mapping interactif V27 sont des **sources historiques de tests et de conception**, non une autorité sur la nouvelle sémantique PATCH SH. Ne pas réécrire ces archives.

## 1. Identités, contenu et séparation FX
- Le SH-01A est **maître des sauvegardes de programmes**. XONE conserve un **miroir automatique**, et non un second programme indépendant à écrire sur le SH.
- Dans le profil SH, **un PATCH miroir XONE contient exactement 36 couples (numéro CC Roland, valeur 0..127)**, associés à l'identité du programme SH, à son contexte destination/port/canal et à son statut de complétude. Les 36 numéros de CC doivent être distincts et appartenir au jeu des 36 paramètres de PATCH identifié/qualifié pour ce profil ; ne pas confondre 36 événements et 36 numéros distincts. La validation des numéros réels reste un test ciblé, pas une réouverture des anciens tests généraux.
- Certains des 36 paramètres ne sont **pas directement accessibles sur le XONE** : les mémoriser silencieusement et les utiliser dans la comparaison du miroir ; ne pas inventer de rotatif, mode ou sortie pour eux. Les CC entrants autorisés peuvent actualiser LIVE indépendamment de la présence d'une commande physique XONE.
- Le PATCH miroir n'inclut **aucun FX INST, paramètre AUM, SPOT/CC central de MACHINE paire, mémoire d'effet ni destination FX**. Les pages MACHINE paires 2/4/6/8 sont des alias FX séparés selon D2. Les POTs/FADERs dont le mapping MACHINE pilote le SH restent des commandes LIVE, pas une deuxième banque de PATCH SH.
- Les anciennes structures MACHINE appairées à 42/84 champs ne deviennent **pas** par conversion implicite des PATCH SH à 36 CC. Introduire une version de profil/schéma dédiée, conserver les sauvegardes anciennes sans les réinterpréter silencieusement et définir la migration/compatibilité avant leur activation dans ce profil.

## 2. Programme BASE et trois miroirs — décision du 5 novembre
| Programme en façade SH (numérotation humaine) | Program Change MIDI (0-based) | Emplacement miroir XONE |
|---|---|---|
| 1 | PC0 | BASE, hors des trois miroirs |
| 2 | PC1 | PATCH XONE 1 |
| 3 | PC2 | PATCH XONE 2 |
| 4 | PC3 | PATCH XONE 3 |

Association **par destination SH configurée** (et synthé/owner correct), jamais globale entre plusieurs SH. Le programme 1 et les programmes 5 à 64 (PC0 et PC4..63) restent utilisables et peuvent actualiser LIVE, mais **ne créent, ne modifient ni ne remplacent** de PATCH miroir XONE. Trois slots PATCH XONE. CUE4 rappelle le BASE avec PC0 et restaure ses FX locaux. Les gestes BASE/CUE/LOAD existants conservent leur timing et leur portée XONE ; ils ne constituent pas un ordre d'écriture SH.

## 3. Règle fondamentale — pas de SAVE XONE sur SH
- **Aucune sauvegarde physique MIDI distante n'est réputée possible ni implémentée.** Le seul WRITE définitif est effectué par l'utilisateur sur la façade du SH. Ne jamais coder un SysEx/WRITE supposé.
- **Aucun bouton, appui SAVE ou validation XONE n'enregistre un PATCH SH**. Si une commande générique SAVE XONE est reçue alors que la destination est ce profil SH, la traiter explicitement comme non applicable, sans changement des trois miroirs, sans WRITE fictif et sans LED « sauvegarde réussie ». Le SAVE générique reste valide pour les autres profils.
- Modification d'un paramètre depuis SH ou XONE : actualise le son et les valeurs **LIVE**, pas la copie persistante du PATCH miroir.
- Après WRITE physique, le miroir est mis à jour **automatiquement lors d'un rappel du programme** et de la réception complète des 36 CC. Il n'y a **aucune validation XONE**. Le WRITE lui-même n'a pas de notification MIDI prouvée : ne pas le détecter ni prétendre le confirmer. Si le SH émet spontanément des CC après WRITE, les traiter d'abord comme observations LIVE ; ne pas les interpréter comme preuve de WRITE ni comme capture de PATCH sans rappel identifié.

## 4. Rappel et apprentissage — même comportement depuis XONE ou SH
### 4.1 Rappel initié par le SH
1. SH sélectionne un programme. XONE reçoit le PC et/ou identifie de manière fiable le programme rappelé, ouvre une **nouvelle capture temporaire**, et met à jour LIVE à mesure que les CC de cette epoch arrivent.
2. Pour PC1/PC2/PC3, accumuler les valeurs **des 36 CC distincts du profil**, sous le même identifiant de programme/destination/epoch ; répétitions du même CC remplacent la valeur temporaire correspondante, sans augmenter le compteur.
3. **Seulement si 36/36 ont été effectivement reçus**, remplacer en une opération atomique le miroir XONE correspondant et persister sa nouvelle image. Aucun clic, aucune validation utilisateur XONE.
4. Si la capture n'atteint pas 36/36 (timeout, câble, changement de programme, CC manquant), **ne pas écraser l'ancien miroir** : le miroir est conservé, la capture est abandonnée et son statut de synchronisation est incomplet. Les CC reçus peuvent rester connus dans LIVE, mais ne deviennent pas une sauvegarde.
5. Pour PC0 et PC4..63, actualiser LIVE sans écrire aucun miroir.

### 4.2 Rappel initié par XONE
1. Le geste PATCH/CUE/LOAD autorisé garde sa quantification existante, notamment le **BAR pour CUE/LOAD**, son owner et sa politique STOP. Après admission, envoyer **uniquement le PC1, PC2 ou PC3** sur la sortie SH configurée.
2. **Ne jamais émettre les 36 CC du PATCH miroir après ce PC**, ni avant, ni en fallback si le SH ne renvoie pas ses paramètres. Le SH est la seule autorité des valeurs permanentes.
3. Réceptionner ensuite les CC du SH avec exactement le même mécanisme de capture temporaire 36/36 que lors d'un rappel depuis sa façade. La réception complète met à jour LIVE et le miroir correspondant. Les PC en écho identiques à la demande en cours ne doivent ni dupliquer ni interrompre l'epoch.
4. Absence de retour/liaison SH : rapporter « non synchronisé »/« retour incomplet », sans faux succès matériel ni émission corrective. La sortie MIDI virtuelle S7 et les états métier génériques restent indépendants de la disponibilité du SH.

### 4.3 Cohérence de l'epoch et du LIVE
- Un seul programme/capture actif par destination SH. Un changement vers un **autre** programme invalide la capture temporaire précédente (sans effacer les copies persistantes) ; ne pas attribuer au nouveau programme des CC observés pour l'ancien.
- Éviter les boucles MIDI : SH -> LIVE/miroir ne réémet pas automatiquement les CC vers SH. Ne pas compter les doublons/échos comme CC supplémentaires. Les PC supplémentaires observés pendant les anciennes sondes ne deviennent pas une contrainte de cardinalité du protocole.
- Pickup : reprendre le système XONE existant avec valeur LIVE fiable, sans nouvelle logique propre au miroir ni saut au retour sur une commande physique.
- Distinguer **LIVE (état instantané)**, **capture 36/36 en cours (volatile)**, **PATCH miroir validé (persistant)** et **commande PC émise (non ACK)**. Ne jamais qualifier de WRITE confirmé une observation de CC.
- Le « 36/36 » vise le jeu de 36 identités CC du profil. Si le jeu exact n'est pas encore qualifié, le moteur peut être construit avec une table de profil configurable, mais aucune capture ne sera déclarée complète par hasard avec 36 CC arbitraires.

## 5. États fonctionnels et comportement UI
- Un PATCH miroir peut être **EMPTY / VALID / SYNC_INCOMPLETE**. Après modification LIVE, l'affichage peut distinguer CURRENT/ALTERED par comparaison, sans changer la sauvegarde.
- Une nouvelle capture 36/36 devient VALID après remplacement atomique, y compris si les valeurs diffèrent de l'ancien miroir. Aucun contrôle manuel de confirmation.
- La sélection du programme 1 ou d’un programme 5 à 64 ne change pas la validité des miroirs 1..3.
- Les états LED, PRESET, RED, MEMORY, RESET/BASE et les banques des **autres profils** restent régis par leurs contrats. Les actions PRESET/RED ne déclenchent ni WRITE ni réémission automatique des 36 CC SH. Ne pas inventer de sauvegarde dans les FX.

## 6. TODO d'implémentation
- **SH-M01 — Profil :** table des 36 identités CC Roland du PATCH et mapping MIDI/port/canal versionné, séparation des 36 du reste du SH. Confronter aux résultats V12 et à une capture propre PC1/2/3 ; documenter seulement ces identités.
- **SH-M02 — Réception réelle AU :** PC/CC SH -> XONE, destination/epoch, staging 36/36, anti-écho, LIVE/pickup, PC1..3 vs PC0 et PC4..63, abandon sans mutation en cas d'incomplet.
- **SH-M03 — Rappel :** XONE -> SH PC **uniquement** après geste autorisé ; SH -> XONE CC de retour, aucun CC forcé, timing BAR et STOP conservés ; SH physique et XONE utilisent le même mécanisme.
- **SH-M04 — Stockage et UI :** trois miroirs persistants exacts, atomicité, protection des anciennes structures 42/84, état EMPTY/VALID/INCOMPLETE/ALTERED ; neutraliser SAVE XONE uniquement pour SH, pas de succès fictif.
- **SH-M05 — Preuves :** tests natifs/host, CI, puis validation AUM + SH physique ; ne pas confondre sonde Mozaic avec test de l'AUv3.

## 7. Tests d'acceptation (à créer dans le dépôt)
- **SH36-T01** : réception PC1, 36 numéros CC attendus distincts -> PATCH1 enregistré automatiquement, LIVE actualisé, aucune action SAVE XONE.
- **SH36-T02** : PC2 ou PC3 -> PATCH2/3 seulement ; PC0 et PC4..63 -> LIVE sans aucune écriture de PATCH1..3.
- **SH36-T03** : 20/36, mauvais canal, câble débranché ou nouveau PC avant fin -> aucun écrasement de miroir ; état incomplet, autres miroirs intacts.
- **SH36-T04** : CC répété, CC étranger, PC écho, rafale inversée, valeurs 0/127 -> pas de faux 36/36 et pas de double capture.
- **SH36-T05** : réglage VCF depuis XONE ou SH -> LIVE modifié, miroir conservé, statut ALTERED si applicable ; FX INST et PATCH des autres profils inchangés.
- **SH36-T06** : SAVE physique SH puis rappel PC1 -> miroir mis à jour si 36/36 ; aucune détection/affirmation de WRITE sans rappel.
- **SH36-T07** : rappel XONE PATCH1..3 -> **PC seul**, au timing contractuel ; pas de réémission CC36, même si miroir ancien ou retour absent.
- **SH36-T08** : geste SAVE générique sur profil SH -> aucune mutation du miroir, aucune confirmation de succès, aucun WRITE ; SAVE de l'autre profil encore opérationnel.
- **SH36-T09** : reopen AUM -> sauvegardes miroirs VALID conservées, invalidation des captures incomplètes, pas de replay automatique du PC/WRITE/CC36.
- **SH36-T10** : vérification physique AUM/SH : WRITE SH, quitter puis rappeler programme ; 36/36 capturés, même son, test de resélection du même programme (le comportement exact reste à établir). Vérifier séparément que WRITE conserve un paramètre réglé par CC XONE.

## 8. État de reprise / bloqueurs réels
- **Le développement peut démarrer sur ce contrat sans nouveau débat de principe.** Sont connus : émission/réception MIDI via le banc Mozaic, choix « 36 CC », 3 emplacements et autorité SH, absence de WRITE MIDI documenté. **Aucun succès de l'adaptateur XONE AUv3 n'est revendiqué.**
- À confirmer avant d'affirmer la fonctionnalité terminée : **la liste exacte des 36 numéros CC et leurs rafales par programme**, **le rappel du programme déjà actif**, **WRITE physique conservant les commandes reçues depuis XONE**, et **la vraie réception/routage AUv3 dans AUM**.
- Une absence de résultat au test « même programme rappelé » ne bloque pas le code : utiliser un aller-retour par un autre programme et retour comme procédure utilisateur/test, sans changement automatique de programme non demandé.
- Ne pas modifier `main`, les archives gelées, ni les worktrees non publiés. Branche de code candidate : `feature/native-e3-bar-beat-reprise`.

## Sources et statut documentaire
- `04_ADDENDUM_SH01A_MACHINES_PAIRES_2026-11-04.md` : D2 MACHINE paire et intégration SH, désormais précisé ici.
- Rapport terrain 1/11, **historique pré-décision** : https://drive.google.com/file/d/1u3uMlEadFoqlhBp85DZ_NzvfOpVfttN8/view
- Handoff Codex V2 1/11, **historique pré-décision** : https://drive.google.com/file/d/1nEfVyFG5L6peG47gkgQ9cb0ktBDT1yiz/view
- Référence API S7 V1.2 : https://drive.google.com/file/d/18GJYIWt8MC0-pWFpl6TR0WYfcWe4fSbh/view
