# Reprise globale XONE — audit, faisabilité et plan du 4 novembre 2026

## Référence et limites de preuve

Inspection du code dans `XONE_AUV3_GLOBAL_20261104`, branche `reprise/global-20261104`, créée à `63e5516f910d8a93444cc67ba3c805762fb0ea82` depuis la branche distante `feature/native-e3-bar-beat-reprise`. Les douze commits depuis `aa6195c` ne changent que les instructions/documentation. Le moteur audité est donc celui de `aa6195c`, avant PR16. Le second kickoff fourni dans le chat est une référence, pas un changement d'ordre : A/B/C d'abord, production ensuite.

Lecture : méthode simple V2, entrée stable et AGENTS S7, kickoff 06 intégral, AGENTS natif, contrats universels 00/01/02, contrats 04/05, audit Drive du 02/11, rapport terrain SH V11/V12 et matrice owners. Les références historiques restent intactes. Aucune campagne AUM, iPhone, K2 ou SH exécutée ici.

### Forensics Windows réellement exécutée

| Worktree préexistant | Branche/HEAD | Travaux locaux observés, conservés |
|---|---|---|
| XONE_ENGINE_NATIVE | feature/native-integration-v3 / c095755 | test_host_musical_clock.cpp non suivi |
| XONE_AUV3_REPRISE_20261027 | feature/native-e3-bar-beat-reprise / 09b30ca | CoreV3, wrapper, HostEngine, test_fx_s6, docs modifiés ; MachinePatchBank.hpp, test_host_machine_patch.cpp, pilotage et preuves non suivis |
| XONE_AUV3_BLOCK2_20261028 | fix/reset-filter-cpu / 584eacd | deux fichiers documentaires modifiés ; nombreux exécutables/logs de preuve non suivis |
| XONE_AUV3_V0_CANDIDATE | detached / 2d8381f | deux fichiers documentaires modifiés, universel_20261028 non suivi |
| XONE_UNIVERSEL_WORK_20261028 | detached / fb173bc | CoreV3, ContinuousInput, HostEngine, quatre tests et CSV modifiés ; test_host_universal_cc.cpp non suivi |

`git stash list` vide. Remote origin : pacomebommier-dot/xone-auv3-test. `ls-remote` constate candidate 63e5516, fix 584eacd, main 5874ae2. Les worktrees anciens ne sont ni nettoyés, ni déplacés, ni publiés. Leur code non commité n'est pas intégré implicitement : le moteur distant contient des implémentations ultérieures, à comparer avant toute récupération ciblée. Source de production unique pour cette reprise : nouveau worktree isolé ; sauvegarde des travaux anciens par conservation intégrale à leur emplacement. Aucun checkout/rebase/reset/clean sur eux.

Dans le nouveau worktree, Google Drive/Git signale plusieurs fichiers M alors que `git diff --stat`, `--numstat` sont vides et `git diff --quiet` retourne 0. Ne pas confondre ces marques de métadonnées avec les vrais changements anciens. Ne pas normaliser les sources gelées pour nettoyer ce statut.

PR16 lue via connecteur GitHub : ouverte, draft, non fusionnée, head 584eacd, base feature/universal-block5-integration / aa6195c ; mergeable au moment du contrôle. Un seul commit, douze fichiers. CI exacte 41129974228 : completed/success. Pour aa6195c le connecteur renvoie 40870666698 completed/success (le rapport ancien citait 40872524738 : ne pas recopier ce numéro comme contrôle actuel).

## A — Audit transversal de l'état réel

Qualifications : CODE = démontré par lecture ; TEST = scénario existant identifié, résultat de cette reprise indiqué séparément ; RISQUE = effet externe ou concurrence non reproduits ; CONTRADICTION = règles incompatibles. P1 = sécurité musicale/intégration ; P2 = capacité, ergonomie ou documentation. Aucun P0 matériel inventé.

| ID/domaine | Constat et preuve vérifiable sur candidate | Qualification / gravité | Solution minimale et responsable |
|---|---|---|---|
| Architecture S1–S8 | CoreV3/CoreCoordinatorV3.hpp:25 `onMidi` stage surface/gestures/NAV puis commit ; HostEngine unique autour de ce core ; OutputS4 gère dettes. Pas de moteur audio ni deuxième scheduler | CODE ; acquis logiciel, matériel non testé | Conserver chaîne, rôle S8 = configuration/propriétaires, pas une 8e instance AU |
| Gestes/NAV/owners | CoreV3 onMidi/onTimer, Core/S3LongTransaction.hpp, S3TouchNav.hpp ; destinations capturées DOWN, publication S3, vrai UP ; tests surface/top_context_capture/touch_flash | CODE + TEST existants ; acquis partiel | Régression après chaque changement NAV ; tests LONG/STOP/owner |
| BAR/BEAT/STOP | XoneHostGrid.hpp authentifie grille hôte ; Scheduler capacité35/refus, mémoire NextBeat, source NextBar ; wrapper livre grille avant MIDI au même sample | CODE + TEST grid/scheduler/memory/cueload | Réutiliser ordre et scheduler ; aucun timer musical SH supplémentaire |
| C01 overflow | Extension/XoneMidiAudioUnit.mm:333 boucle `processed < 256` ; fin appelle seulement renderOverflow ; HostEngine:1221 compteur/trace, aucune reprise des UP/FC restants | CODE, défaut de conception P1 ; perte matérielle non reproduite | Test vraie AU >256 incluant UP/FC, stratégie de surcharge sûre et bornée ; wrapper |
| CC/pickup | XoneContinuousInput.hpp continuousTarget sépare cellule et API ; pickup par API931 + références physiques20 ; refus UNKNOWN | CODE + TEST continuous/universal_roundtrip | Alias FX doit rendre la même identité, jamais un second pickup |
| SPOT/TOUCH/FLASH/TOP/BOT | Host spots/top capture, OutputS4 debt/reserve, TopPulseBus FIFO ; callback refusé ne vaut pas SENT ; test_host_touch_flash/stop_retries/top_* | CODE + TEST ; matériel non testé | Préserver OFF et capture, tester surcharge et changement de vue tenu |
| DJ/LOOP C05 | HostEngine pendingDjLoopService et BrowserLoadRequest ; wrapper ne consomme aucun backend ; notes/CC virtuels et gains LOOP sont distincts | CODE ; service externe non raccordé P2 | Conserver API virtuelle ; ne pas annoncer chargement de deck/lecture effective |
| MUTE/SOLO | GlobalMute, MuteBarBridge, projection MIDI/LED, test_host_mute_* ; états et quantification présents | CODE + TEST ; acquis logiciel | Régression combinée avec PRESET/RED et STOP |
| PATCH/MEMORY/CUE/BASE | S8Sample28, S8Machine30, Source60 ; HostEngine:328–378 mémoire BEAT/source BAR, capture owner ; test pair_patch/local_patch_save/source_recall | CODE + TEST ; pas fonctions absentes faute d'instrument | Préserver sauvegardes génériques et exclusions ; spécialiser SH seulement |
| PRESET/RED | XoneGlobalSnapshot.hpp:25 inScope ; HostEngine captureGlobal/applyGlobal ; tests global_snapshot et universal_roundtrip | CODE + TEST ; pas ACK instrumental | Ajouter frontière SH commune à toutes restitutions nominales |
| C02 SH | HostEngine:581 câble non0 UNTRUSTED, length!=3 refus (PC longueur2), observed:1189=-2 ; profile SOURCE_VERIFIED_NOT_BOUND, port/canal/bindings vides | CODE, extension manquante P1 | Adaptateur explicite par destination/canal, entrée PC2/CC3, aucun apprentissage depuis route arbitraire |
| C03 PR16 | Diff aa6195c..584eacd : RESET configurés, commitNativeReset LONG, defaults CRC951, borne MEMORY et publication dirty ; pas présent dans candidate | CODE, intégration P1 | Intégrer sans archives après régressions ciblées ; pas main |
| C04 config | APIs S8/alternative/INIT internes ; wrapper UI preset/refresh/diagnostic seulement | CODE, lacune P2 | Configuration minimale versionnée ; UNKNOWN reste correct, pas defaults inventés |
| C06 UI | XoneAUViewController.m UIKit minimale effectivement présente ; moniteur complet absent | CODE, partiel P2 | Hors changement musical immédiat ; ne pas déclarer maquette livrée |
| C07 SAMPLE | GlobalSnapshot::inScope exclut modes SAMPLE >=28 comme MACHINE >=30 ; test_host_global_snapshot.cpp:50 exige exclusion SAMPLE ; note04 dit même chose mais maître00 interdit généralisation | CONTRADICTION + CODE P1 | Ne pas changer modes SAMPLE par analogie ; retrouver décision S8 spécialisée avant correction, exposer divergence |
| C08 CPU/alloc | HostEngine advance:1194 et onMusicalGrid:408 mettent dirty même sans mutation ; endRenderBuffer:1049 copie document ; allocation unique DocumentExchange au constructeur ; test_realtime_budget surveille allocations C++ (pas runtime ObjC) | CODE, coût idle corrigé dans PR16 ; appareil RISQUE P1 | Intégrer PR16, mesurer ensuite rappels/pics/xruns en AUM ; pas gain AUM x29 déduit de Windows |
| C09 désallocation | Wrapper deallocate:84 stop puis callbacks nil ; OFF conservés, premier rendu après réallocation les réessaie ; retrait définitif n'a pas de callback suivant | CODE, limite cycle vie P1 | Recette STOP/vrais UP/OFF avant retrait ; ne pas envoyer depuis callback hôte hors rendu sans contrat |
| C10 docs | Note04 prétend persistance non faite alors que DocumentImage archive S8 ; anciens compteurs/README prototype ; contrats SH05 supersèdent SAVE/LOAD V11 | CONTRADICTION/obsolescence P2 | Ce rapport = entrée technique de reprise ; ne pas réécrire archives |
| C11 mailbox/persistance | Session StateMailbox:55 triple buffering SPSC, slots exclusifs, wrapper synchronized sérialise UI ; Document V2 CRC, max131072, validation avant sortie ; tests document/session/AU restore tenus | CODE + TEST, concurrence AUM RISQUE P1 | Garder un seul producteur/consommateur ; aucun accès UIKit direct au core ; stress save/restore |
| GCC16 baseline | test_host_document compile O2/Werror échoue sur avertissement array-bounds applyDocument:1710 (index -1 converti unsigned analysé) | TEST exécuté, échec build P1 ; pas preuve crash runtime | Correction de borne déjà dans PR16 ; conserver log RED puis rejouer sans désactiver warning |
| AUM | RESULTATS_AUM_V0.csv E00–E12 tous NON_TESTE | CODE/document ; validation bloquée par absence d'essai, pas code bloqué | Recette appareil finale distincte, aucun PASS inventé |

Contrôles effectivement exécutés avant intégration : verify_source.py PASS (12 headers gelés/14 suites,81REQ/382handlers), verify_simulator.py PASS, verify_profiles.py PASS (42 CC inventoriés). Le premier test ciblé document échoue à la compilation GCC16 ci-dessus ; le banc s'arrête et n'annonce pas les autres suites PASS. Source log : evidence/reprise-20261104/test_host_document.compile.log.

## B — Faisabilité SH / MACHINE et recherche d'impasses

### Verdicts techniques

| Risque / condition précise | Preuve et verdict | Solution minimale, tests / régression |
|---|---|---|
| SH36 face à30/60 | S8Machine::ParamCount30, Source::Fields60, documentFields exige leur forme. FAISABLE AVEC ADAPTATION ; remplacer les tailles détruirait compatibilité | Trois images36 dédiées par destination/profil versionné ; pas migration implicite 42/84/30/60 ; champ AU indépendant versionné comme defaults PR16 ou nouvelle version décodeur explicitement compatible. Tests ancien V2 inchangé, CRC, identité, restauration atomique |
| Identités36 exactes | Profil source contient42 CC, V12 rapporte36 sur72 événements et9 PC ; seulement5/3/12/13/14/15 explicitement relevés. NON DÉMONTRÉ ; ni36 premiers ni36 arbitraires admissibles | Table36 configurable validant distincts/plage/provenance ; refuser activation qualifiée sans jeu confirmé ; chercher capture V12 complète puis recette isolée PC0/1/2. Développement staging testable avec profil synthétique clairement marqué |
| PC local puis feedback sans PC | PC émis n'est pas ACK ; retour SH local peut ne pas renvoyer PC. FAISABLE AVEC ADAPTATION | Epoch démarrée au callback PC réussi, pas au simple armement BAR ; statut attente ; PC entrant même/différent et timeout explicites ; échec output ne démarre pas capture ; T03/T04/T07 |
| Échos/doublons | CC contient canal,numéro,valeur ; aucun ID de programme ou epoch. Même CC répété ne peut augmenter cardinalité. FAISABLE pour déduplication ; identité physique dépend routage | Masque36, chaque numéro attendu compte1, dernière valeur staging ; route retour dédiée sans thru automatique ; PC répété ne doit pas arbitrairement effacer une capture. Tester PC écho avant/milieu/après, aucune réémission RX |
| Ancien CC retardé après nouveau PC | Séquences PC0,20CC,PC1,16 anciens CC,20 nouveaux peuvent donner36 numéros sous mauvaise epoch ; les octets sont indiscernables d'une réponse légitime PC1 | IMPASSE à une garantie absolue sur flux contaminé non identifié. Ni timer ni compteur ne prouve provenance. Exiger routage sans boucle et séquence firmware qualifiée ; annuler capture ambiguë, ancien miroir intact ; alternative compatible : rappel explicite isolé et reprise après interruption, sans programme automatique |
| Édition LIVE mêlée à réponse | CC d'édition et CC stocké même format. NON DÉMONTRÉ que36 suffisent à prouver image sauvegardée sous édition simultanée | Invalider capture sur édition locale pendant collecte ; recette sans édition pendant rafale ; ne jamais appeler réception preuve WRITE. Édition physique simultanée reste limite intrinsèque sans marqueur SH |
| WRITE physique puis même PC | V11/V12 ne prouvent ni WRITE conservant CC XONE ni réémission complète au même programme. NON DÉMONTRÉ, pas impossibilité SH déclarée | T06 simulé prouve mécanisme seulement ; T10 physique WRITE, quitter puis rappeler, extinction/reopen et CC XONE ; alternative déjà autorisée aller-retour manuel, pas automatique |
| Autres programmes | PC3..63 autorisés mais ne possèdent aucun slot miroir. FAISABLE | LIVE toujours observé, staging ancien annulé ; trois slots seulement ; T02/T03 |
| SAVE générique SH | saveSourcePatch et saveMachinePatchFromLive sauvegardent générique ; TOP+BOT consomme geste. FAISABLE AVEC ADAPTATION | Garde profile/owner dans point commun avant capture, statut non applicable sans LED succès ; ne jamais fallback rappel ; autres instruments inchangés T08 |
| Toutes voies RECALL | SourceRecall applique60 et reset37 FX ; applyGlobal et applyDocument parcourent LIVE nominal ; AlternativeBases peut émettre97. Aucun simple remplacement de TOP ne protège PRESET/RED/reopen | FAISABLE AVEC ADAPTATION : politique de destination partagée au moment composition des sorties, SH PATCH PC seul ; PRESET/RED/restaure ne rejouent pas CC36/WRITE. MEMORY FX reste FX, BASE SH jamais0 Roland présumé. Tests chaque voie et autre source |
| Pickup/paramètres non physiques | Pickup existant par API ; 36>18 contrôles/pAGE. FAISABLE | Garder toutes36 dans état matériel profil ; publier vers S3 seulement bindings virtuels connus, rearm seule cible modifiée ; pas cellule arbitraire pour CC sans rotatif ; T05 |
| Transport/ports AU | câble AU != identité appareil ; un output virtuel actuel ; entrée refuse câble!=0. FAISABLE AVEC CONFIGURATION | Métadonnées route/canal explicites séparées K2 CH15/feedback SH ; vrai PC2 bytes ; aucune authentification déduite d'un câble ; test vraie AU puis AUM |
| CPU/persistance SH | 108 valeurs + staging36, masque, IDs ; aucune nécessité nouveau scheduler/transaction réseau | FAISABLE ; tableaux fixes, publication sur miroir changé uniquement ; volatile epoch exclue persistance, reboot sans émission ; T09 et tests allocation |
| MACHINE identité source | Matrice owners : paires1/2 INST1,3/4 INST2,5/6 YELLOW1,7/8 YELLOW2 ; NavS3 overlay:236 route actuellement tout centre MACHINE | FAISABLE AVEC ADAPTATION ; source=(page-1)/2, conserver alternative active. Aucun détournement des FX vers SH |
| MACHINE12 vs FX24 | Chaque source possède24CC +12états SPOT (sixCC/troisSPOT par sous-page) ; natif3 et emprunté3 sont deux côtés des sixCC, pas24 nouveaux paramètres | FAISABLE ; matrice physique colonne/ligne vers quatre pages, offsets0..2/3..5 à vérifier contre NavS3 réel. SPOT famille/mode peut différer côté emprunté : ne pas additionner un offset arbitraire à l'état. Test comparer route INST existante plutôt que table inventée |
| MACHINE LONG | S3LongTransaction/S3TouchNav gardent archives NAV, NavS3::validate régénère routes déterministes ; ajout host seul désaccorde owner/LED | FAISABLE AVEC ADAPTATION S3 : état de vue NAV explicite validé, SHORT entrée native, LONG déjà actif bascule une fois, vrai UP, pas SHORT+LONG ; tenir SPOT capture ancienne destination jusqu'àOFF ; MP02/03/06 |
| Exclusion PATCH paires | Source60 aujourd'hui inclut champs0..29 de chacune ; page paire centre0..11 et18..29 doit sortir mais12..17 rester | FAISABLE AVEC ADAPTATION ; masque de domaine, pas resize silencieux ; ancien document conserve données historiques mais alias ne les restaure pas dans FX. Modes MACHINE de zone retirée ne deviennent pas modes FX ; MP05 + ancien V2 |
| État unique FX | ContinuousPickup parAPI et LIVE cellules1552..1647/1768..1815 ; LED derive NAV/route | FAISABLE ; même route canonique pour CC/SPOT/LED/reset/observations, aucune seconde mémoire/pickup. MP01 toutes8pages, MP03 modes+2vues, MP04 bidirectionnel, MP06 PRESET/RED, MP07 vraie AU/AUM |

L'impasse démontrée porte sur la preuve de provenance de CC non marqués dans un flux intercalé, pas sur la réalisation du contrat dans un routage qualifié. Aucun protocole WRITE/SysEx supposé. Ne pas promettre de résoudre cette information absente par un timeout.

### Scénarios de preuve à exécuter

SH36-T01..T09 : jeu de profil explicitement qualifié ou fixture synthétique nommée comme telle ; comparer miroirs avant/après, LIVE et tous octets sortants. Ajouter contre-exemple intercalé ci-dessus, CC étrangère/duplication, PC écho, erreur callback PC, STOP entre BAR et retour, changement owner/alternative, capture traversant buffers, timeout, retour après reopen. SH36-T10 reste matériel obligatoire.

MP01..MP07 : générer chaque paire et comparer chaque route aux deux vues INST natives existantes ; tenue TOUCH/FLASH durant LONG/page/STOP, RESET SPOT, SAVE/RECALL n'écrit aucun FX depuis payload ancien, PATCH SH PC seul avec FX depuis MACHINE2 simultané ; même API => même pickup/LED. MP07 physique non validé par source.

## C — Plan de reprise priorisé, sans refonte

1. **En cours : intégrer PR16** dans cette reprise isolée depuis63e5516, sans changer main ni les cinq worktrees ; test RESET/idle/document/stop/touch/pair/global, intégrité puis suite complète. Le warning GCC16 baseline constitue une reproduction utile de sa borne MEMORY.
2. **V0 P1 : wrapper overflow** : test vraieAU 257+ avecUP/FC en queue ; conserver ordre musical et OFF, pas simple hausse de256. Examiner budget de parcours hôte et cas de saturation avant choix. Désallocation finale : recette explicite plutôt que callback horscontrat.
3. **V0 contrat SAMPLE** : localiser décision S8 spécialisée ; tant que contradiction subsiste ne changer ni SAMPLE ni preuves ; les autres travaux avancent. BASE4 GLOBAL/LOOP reste à compléter selon defaults prouvés, pas0 arbitraire.
4. **SH** : profil36/config + staging/version, réceptionAU PC/CC et publicationS3, rappelPC via scheduler existant, garde SAVE et toutes restitutions, persistance indépendante compatibleV2 ; tests SH36-T01..T09 puis recetteT10. Qualification liste36 requise pour annoncer synchro terminée, pas pour écrire tests du mécanisme.
5. **MACHINE paires** : matrice routes comparée àNavS3, vue NAV/LONG, exclusion payload via masque compatible, unique pickup/LED ; MP01..MP06 puis vraieAU/AUM MP07. Aucun nouveau moteurFX.
6. **Palier combiné** : CI Linux GCC/Clang, AppleClang, ASan/UBSan, harness vraieAU et buildiOS sur SHA publié ; remplir seulement résultats réellement exécutés. Recette AUM iPhone/K2/SH : route exclusive K2/SIM, retourSH dédié, CCOUTPUTON, ports/canal, rafales, WRITE+rappel, STOP/retrait/reallocation, save/reopen, CPU/pics/xruns interface ouverte/fermée.

Pas de GO requis entre étapes autorisées ; pas de merge main. Les contradictions et manques matériels sont circonscrits et ne gèlent pas la V0.

## Sources consultées

- PR16 https://github.com/pacomebommier-dot/xone-auv3-test/pull/16 ; CI https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/41129974228
- Audit02/11 https://drive.google.com/file/d/1A1_MyT2ON-voy5_dDHZLNhZFGHjBZQ2_/view
- RapportV11/V12 https://drive.google.com/file/d/1u3uMlEadFoqlhBp85DZ_NzvfOpVfttN8/view (anciennes règles SAVE/LOAD supersédées par05)
- Matrice owners https://drive.google.com/file/d/1GUt38gN8dV5PalylmMLSMGRRlztTj6my/view (zones paires supersédées par04D2)
