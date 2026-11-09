# KICK-OFF CODEX — REPRISE GLOBALE XONE AUv3 + AUDIT DE FAISABILITÉ SH/MACHINE
**Mandat utilisateur : 4 novembre 2026.** Ce document remplace le KICK-OFF trop étroit « SH-M01 d'abord ». Il complète le kick-off WORK du 04/11 et les contrats universels, SH et MACHINE. **Audit général demandé explicitement**, puis reprise immédiate du développement, sans laisser l'audit geler la production.

## 0. Objectif et ordre
Reprendre **le chantier XONE déjà avancé et interrompu** : moteur MIDI universel, entrée/sortie AUv3, gestes/NAV/owners, PATCH/MEMORY/CUE/LOAD/BASE, PRESET/RED, FX/MACHINE/SAMPLE, LED, STOP/NoteOff, CPU, persistance, recette AUM. **Ne pas réduire XONE à un programme SH.**

Fournir d'abord un **audit général transversal** de l'état effectif du code actuel et une **étude de faisabilité rigoureuse** des deux retouches SH-01A et MACHINE paires : contradictions, collisions, impasses (« dead ends »), compatibilité des données, risques MIDI et rendu temps réel, tests de preuve et solutions. Puis **reprendre et coder** selon une séquence justifiée et des commits/CI contrôlés. L'audit n'est pas l'autorisation d'une refonte du moteur : réutiliser le natif existant et éviter l'analyse répétitive des lots historiques.

## 1. Références / état distant constaté lors de la préparation
- Dépôt : `pacomebommier-dot/xone-auv3-test`. Branche candidate : `feature/native-e3-bar-beat-reprise`. HEAD distant documenté : `64c27e7316a4e7756c702c902f07ac7639e1bd07` ; **les onze commits qui suivent `aa6195c` modifient exclusivement de la documentation, pas le moteur**. Toujours revérifier ces SHAs au démarrage, car une copie locale peut contenir des travaux ultérieurs.
- Dernière candidate **fonctionnelle** établie : `aa6195c3b6c91021f0e5fba332a9268ccbd983bb`, CI 29/10 PASS. Fiche AUM `docs/reprise/RESULTATS_AUM_V0.csv` : E00–E12 NON_TESTE au dernier contrôle. CI PASS ne prouve ni usage AUM ni SH.
- PR #16 `fix/reset-filter-cpu`, commit `584eacdf73427f9ce1551fb2611c9ecd2836e3c5` : correction RESET/FILTER et coût CPU ; CI 30/10 PASS. **Non présente dans le HEAD candidate comparé à cette préparation ; branches divergentes.** Examiner diff, conflits, tests et travail non publié avant toute intégration ; ne PAS merger dans main.
- `main` (prototype ancien) n'est PAS le point de départ.
- Références Drive : `KICKOFF_WORK_XONE_V0_20261104.md`, `AUDIT_CODE_XONE_V0_PR16_20261102.md`, `00_LIRE_EN_PREMIER_XONE.md`, registre d'autorité et rapports SH V11/V12.
- Hiérarchie GitHub : `AGENTS.md`, `docs/universel/00_CONTRAT_MAITRE_UNIVERSEL_V1.md`, `01_CONTRAT_API_ET_ADAPTATEURS_V1.md`, `02_PLAN_BATAILLE_5_BLOCS_V1.md`, `04_ADDENDUM_SH01A_MACHINES_PAIRES_2026-11-04.md`, **`05_CONTRAT_SH01A_MIROIR_PATCH36_V1_20261104.md` prioritaire pour le seul profil SH**. Les rapports du 1/11 ne sont plus normatifs sur le SAVE/RECALL SH.

## 2. ÉTAPE 0 — Forensics ciblée du travail interrompu (OBLIGATOIRE)
Sur la machine de CODEX, inventorier `pwd`, `git status -sb`, `git rev-parse HEAD`, `git branch -avv`, `git worktree list --porcelain`, `git stash list`, remotes, changements non suivis/ignorés significatifs, commits non publiés, CI et PR en cours. Repérer le worktree actif réel, pas une vieille copie sur Drive. **Sauvegarder tout dirty/unpublished avant checkout/merge/rebase/reset/clean.** Ne pas écraser, supprimer ou publier le code non examiné. Si l'environnement ne voit pas Windows, indiquer explicitement cette limite ; ne pas affirmer un `git status` local non exécuté.

Résultat de cette étape : source unique de travail, SHA exact, branches/copies en conflit, dernières modifications réelles, tâches interrompues ; comparaison aux derniers rapports. **Ne pas redémarrer par SH-M01 tant que l'état réel de la V0 n'est pas identifié.**

## 3. ÉTAPE 1 — AUDIT GÉNÉRAL GLOBAL, SUR LE CODE RÉEL
Audit obligatoire, **transversal mais sans ré-auditer indéfiniment les archives L01–L16** :
A. Architecture S1–S8/CoreV3/Host/Extension, exclusivité du producteur d'état, source K2 vs K2_SIM, chaîne S3→S4→MIDI, owners, navigation, gestes LONG/SHORT, CLOCK BAR/BEAT et capture des destinations.
B. API MIDI virtuelle S7 (canaux 7–14, LED 15), dispatch CC/notes, NoteOn/NoteOff, TOUCH/FLASH/TOP, DJ, LOOP, MUTE/SOLO, STOP et callback refusé/absent ; aucun routage audio dans XONE.
C. Banques et opérations PATCH/MEMORY/CUE/LOAD/BASE/PRESET/RED : 3 slots par owner, EMPTY/CURRENT/ALTERED/UNKNOWN, quantification, reset/refus sûrs, exclusions modes SPOT S8, cohérence lors d'un changement d'owner.
D. Wrapper AUv3 et temps réel : gestion du plafond **256 événements par buffer** (perte potentielle de NoteOff/STOP), dépassement de tableaux/queues, pas d'allocation/copie excessive dans callback, buffers/epochs, STOP lors de la dernière désallocation, CPU idle (PR16), flux MIDI entrant authentifié.
E. Persistance et UI : `fullState`/`setFullState`, CRC/document V2, migrations, save/reopen, blocage de replay transitoire, mailbox UI/audio, LED physiques/non prouvées, décalage entre UI UIKit minimale et maquettes.
F. Configuration future/UNKNOWN : ne pas classer les sorties virtuelles comme manquantes au seul motif qu'un plugin ou SH est non configuré ; ne pas transformer UNKNOWN en 0.
G. Comparer code + tests + rapports et CI. Qualifier chaque constat : **DÉMONTRÉ PAR CODE**, **COUVERT PAR TEST**, **RISQUE À TESTER**, **CONTRADICTION CONTRACTUELLE**, ou **IMPOSSIBILITÉ MATÉRIELLE**. Éviter les « bugs » imaginaires.

Points de départ explicites de l'audit 02/11 : C01 overflow256, C02 feedback SH absent, C03 PR16 à intégrer, C04 configuration S8/defaults, C05 DJ request-only, C06 UI absente, C07 divergence SAMPLE vs MACHINE sur modes SPOT, C08 CPU AUM non mesuré, C09 OFF final après désallocation, C10 pilotage périmé, C11 mailbox/persistance. Vérifier **ce qui reste vrai après l'inspection du HEAD**, ne pas copier mécaniquement l'ancien verdict.

## 4. ÉTAPE 2 — ÉTUDE DE FAISABILITÉ DES NOUVELLES RÈGLES : CHERCHER LES DEAD ENDS
### SH-01A — CONTRAT 05, exclusivement ce profil
Décisions produit non négociables sans rapport explicite de contradiction : SH maître des programmes, WRITE physique uniquement ; XONE 3 miroirs (SH1/2/3, PC0/1/2), 36 CC distincts par miroir ; aucun bouton SAVE XONE ; modification LIVE ≠ sauvegarde miroir ; rappel XONE = PC seul, **jamais les 36 CC** ; miroir remplacé atomiquement uniquement après réception 36/36 à la suite d'un rappel ; capture partielle ne détruit pas l'ancien miroir ; autres programmes SH actualisent LIVE sans affecter ces 3 miroirs ; aucun FX INST dans PATCH SH. Les autres profils XONE gardent leur SAVE/RECALL générique.

Faire une matrice de faisabilité et POC/test ciblé sur les risques suivants :
1. **Cardinalité / identité** : les 36 numéros CC exacts sont-ils stables, émis lors de chaque rappel identifiable, distincts, et représentatifs d'un PATCH SH ? V12 a vu 36 CC distincts mais dans des fenêtres comprenant plusieurs PC. Le contrat fixe 36 comme cible ; ne pas déclarer une capture complète à partir de 36 CC arbitraires, ni supposer que 36 CC couvrent des paramètres non-MIDI. Différencier commandes reconnues, CC transmis et paramètres réellement sauvegardés.
2. **Délimitation d'une capture** : PC entrant fiable après rappel local ? Si XONE émet PC sans PC de retour, comment démarrer l'epoch ? Bornes de temps/ordre lorsque PC et CC arrivent dans plusieurs buffers, réémission, Soft Thru/échos, doublons, CC d'édition LIVE mélangés à la rafale, rappel interrompu. Ne pas utiliser un timer qui bloque l'audio ; refuser la validation ambiguë.
3. **SAVE réel vs « faux SAVE »** : le WRITE physique n'envoie pas de notification connue ; un rappel retourne-t-il réellement les valeurs stockées, notamment après une édition par CC XONE ? Un rappel du **même programme courant** provoque-t-il un flux de CC ? Sinon proposer un aller-retour manuel entre programmes, sans déclencher automatiquement une sélection non demandée. Ne jamais prétendre qu'un miroir mis à jour prouve le WRITE.
4. **Conflit avec le moteur PATCH existant** : `XoneS8MachinePatchBank` gère 30 valeurs par page, `XoneSourcePatchBank` 60 valeurs par paire, alors que SH exige **trois miroirs de 36 CC** et PC-only. Comparer au document V2, snapshots, owners et modes ; proposer une extension profil/versionnée qui n'altère pas les autres synthés ni les anciens documents. Définir les éventuelles contraintes de migration. Ne jamais remplacer silencieusement les tableaux 30/60 par 36.
5. **PRESET/RED/MEMORY/BASE/CUE/LOAD** : auditer toutes les voies qui pourraient réinjecter les 36 CC ou déclencher SAVE malgré « PC seul ». Identifier l'owner exact, les annulations STOP/BAR, et la règle d'exclusion SH sans désactiver les commandes virtuelles génériques des autres profils.
6. **Vrai transport AUv3 et routage AUM** : code actuel refuse les câbles non0 et `observed()` renvoie UNKNOWN selon audit 02/11 ; vérifier la route réelle SH->XONE, output PC, port/canal, observations fiables, anti-écho. Le banc Mozaic V11/V12 ne prouve pas le wrapper XONE.

### MACHINE paires — CONTRAT 04 D2, pour toutes 2/4/6/8
7. Les **12 CC rotatifs centraux + 12 SPOT** deviennent strictement des **alias des FX INST existants**, répartis en 4 groupes de 3 ; vues native/empruntée, SPOT mêmes modes/états/LED/actions que sur les sous-pages d'origine. Vérifier la **correspondance exacte des indices** : les quatre sous-pages INST ont davantage de paramètres que 12 positions physiques ; ne pas supposer une bijection sans matrice physique→FX sourcée. Pointer chaque position vers l'identité FX préexistante (y compris contexte emprunté).
8. **Collision gestes/navigation** : LONG sur MACHINE paire déjà active bascule de vue ; vérifier seuils, ordre DOWN/UP, SPOT tenu, FLASH/TOUCH actif, changement de page et STOP, impossibilité d'exécuter SHORT+LONG ensemble.
9. **Rattachements PATCH** : la zone centrale paire ne doit plus être capturée par PATCH MACHINE, tandis que POTs/FADERs et autres commandes non requalifiées restent MACHINE ; audit des 30/60 anciennes valeurs, de leurs propriétaires et des snapshots qui risqueraient d'écraser les FX INST ou les paramètres SH. Ne créer ni double mémoire FX, ni nouveau pickup.
10. **Cohérence globale** : FX accessibles par deux surfaces avec unique état LIVE/LED/pickup ; source INST1/INST2/alternatives résolue correctement selon les pairs ; ne pas détourner les effets AUM vers les CC SH. Tester les deux sens et toutes les pages MACHINE paires.

Pour **chaque risque**, rendre : condition exacte, preuve ou test minimal, verdict **FAISABLE / FAISABLE AVEC ADAPTATION / NON DÉMONTRÉ / IMPASSE**, solution minimale, impact code/contrats/tests, nécessité ou non d'essai physique. Si réel dead end : montrer le chemin et les données, proposer au moins une alternative contractuellement compatible ; **ne pas contourner par une invention silencieuse**.

## 5. ÉTAPE 3 — PLAN DE REPRISE ET PRODUCTION
Après rapport d'audit et matrice faisabilité, **continuer sans demander GO** sauf contradiction métier indécidable ou action destructive.

Ordre par dépendance et danger, à confirmer par l'audit :
1. Sécuriser worktree et revoir/intégrer PR16 dans la branche candidate (pas main), puis tests RESET/FILTER/CPU.
2. Corriger risques transversaux P0/P1 déjà prouvés de la V0 : perte MIDI >256, STOP/NoteOff, données persistées, modes SPOT SAMPLE/MACHINE, BASE4 GLOBAL/LOOP selon defaults prouvés. Préserver toutes commandes neutres. Préparer candidate V0 et protocole AUM.
3. Implémenter l'adaptateur SH **après validation de la frontière des 36 identités et des epochs**, par incréments fonctionnels SH-M01..M05 + SH36-T01..T10. Ne pas bloquer tout le projet si un point exige le SH physique ; développer/tests simulés déterministes et classer le test terrain à réaliser.
4. Implémenter la surface FX des MACHINE paires MP-A..D + MP01..MP07 sur les owners/index vérifiés, sans casser PATCH générique ni SH.
5. Repasser tests de non-régression combinés : SH rappel (PC seul) + FX depuis MACHINE2 + PATCH/memory/preset/red/autres instruments, double surface et pickup, STOP, persistance, reconstruction AUM. UI/graphisme reste chantier prévu, non « validé » parce qu'une maquette existe.
6. CI complète par SHA et recette AUM appareil explicitement consignée. Aucune réussite physique inventée.

Garder le plan universel 5 blocs comme traçabilité, sans remettre artificiellement le compteur historique 0/6 ni le remplacer par des micro-lots. Aucune nouvelle architecture de scheduler ni CORE historique réécrit. Commits cohérents publiés, tests ciblés après changements, CI complète sur palier validable ; traiter le premier FAIL réel avant de poursuivre.

## 6. Livrables et premier message exigés
**Livrable A :** `docs/reprise/AUDIT_GLOBAL_ET_FAISABILITE_SH_MACHINE_20261104.md` avec tableau vérifiable (domaine, constat, preuve fichier/ligne ou test, gravité P0/P1/P2, risque vs bug, solution, propriétaire, action). **Inclure les impasses/contradictions plutôt que les masquer.**

**Livrable B :** matrice de faisabilité SH + MACHINE, scénarios de replay testables, décision d'architecture minimale, statut de chaque point non prouvé matériellement.

**Livrable C :** plan d'exécution actualisé fondé sur le HEAD réellement retrouvé et les corrections en cours, **sans abandonner l'avancement V0 existant** ; commencer ensuite le code, publier commits, résultats tests et CI, état AUM.

**Format des comptes rendus :** HEAD/branche/worktree, tâches préexistantes terminées/partielles, écarts confirmés, décisions d'architecture, fichiers touchés, commandes de test/résultats, CI et prochain geste. Pas de conclusions « PASS » sans exécution.

**Commencer par écrire exactement l'état du worktree et de la candidate, puis auditer l'ensemble et étudier la faisabilité SH/MACHINE. Ne PAS commencer par coder SH-M01 en ignorant PR16 et les écarts V0. GO.**
