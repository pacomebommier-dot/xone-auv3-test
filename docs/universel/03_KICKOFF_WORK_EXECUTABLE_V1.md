# KICKOFF WORK/CODEX — XONE UNIVERSEL
Date : 28 octobre 2026. Objectif : continuer à coder sans refaire de grands audits.

MISSION : terminer XONE AUv3 comme moteur de contrôle UNIVERSEL, avec toutes fonctions internes et sorties MIDI neutres testables sans instrument ni plugin musical. Les adapters AUM MIDI Learn et profils MIDI hardware sont des configurations futures. Ne pas les prendre pour des bloqueurs du moteur.

LECTURE OBLIGATOIRE COURTE :
1. docs/universel/00_CONTRAT_MAITRE_UNIVERSEL_V1.md
2. docs/universel/01_CONTRAT_API_ET_ADAPTATEURS_V1.md
3. docs/universel/02_PLAN_BATAILLE_5_BLOCS_V1.md
4. docs/pilotage_v0/00_CONTRAT_PILOTAGE_V0.md uniquement pour non-régression et règles de sécurité, sans sa vieille classification de bindings.
5. Références S7 API et décisions S8 strictement nécessaires au bloc courant. NE PAS relire tout le Drive, ni réouvrir les 16 lots.

PROTECTION : base d'essai identifiable 2cedd1e mais le HEAD actif peut être plus récent. Branche feature/native-e3-bar-beat-reprise : vérifier git status/HEAD avant écriture et préserver tout travail non publié. Ne pas reset destructif. Aucun merge sur main. Archives et empreintes gelées intactes. Ne pas inventer valeurs MIDI, profil Roland, ACK, audio ou preset chargé.

COMMENCER MAINTENANT : BLOC 1/5 — API MIDI UNIVERSELLE ET GESTES.
Faire uniquement une courte comparaison des familles de sorties de S7 V1.2 avec les dispatchs natifs et AU existants. Lister jusqu'à 8 écarts réels, puis CODER les liaisons restantes par fonctions cohérentes. Les commandes virtuelles admissibles doivent arriver au moniteur sans synthé ; les opérations matérielles effectivement inconnues restent explicitement non raccordées, sans bloquer les autres parcours. Exercer S1/S2/S3/S4, owner, STOP/NoteOff, même-sample BAR, TOUCH/FLASH et PICKUP avec tests SOURCE puis wrapper AU.
Poursuivre automatiquement après chaque commit testé, publier les commits, fermer le bloc avec CI complète PASS du même SHA, puis attaquer le bloc 2 sans demander GO. Ne pas inventer de mini-compteur (1A/1B/1C...). Si échec, corriger le défaut local et poursuivre. Si impossibilité d'environnement, enregistrer checkpoint précis (fichier/ligne/test/commande suivante).

RESTANT : BLOC2 banques PATCH/MEMORY/CUE/LOAD/BASE ; BLOC3 PRESET/RED/S8 ; BLOC4 feedback/persistance ; BLOC5 recette universelle AUM. Ne pas introduire de fonction nouvelle hors contrats.

DÉFINITION DE SUCCÈS : moteur universel produit ses sorties de contrôle MIDI7..14, LED séparée MIDI15, états et banques fiables, save/reopen, sans audio ni synthé. AUM réel n'est PASS que sur vérification sur appareil. Le banc L02 CH1 CC4->CC74 et CC8->CC71 n'est pas l'API de production.

PREMIER MESSAGE ATTENDU :
« Bloc 1/5 en cours — API universelle + gestes. HEAD [sha], non-publiés protégés. Fonctions déjà raccordées [bref], écarts bornés [bref]. Je code et valide maintenant. »
Ensuite progresser sans demander d'autorisation après chaque commit, sauf action irréversible, désaccord contractuel véritable ou décision utilisateur indispensable.
