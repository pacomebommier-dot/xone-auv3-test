# XONE V0 universel — recette AUM
29 octobre2026. Identité conservée : aumi/XnLb/Xn02, XONE ENGINE V0.2.
Statut de campagne : **NON_TESTE_AUM**. Le SHA de compilation est celui du COMMIT.txt de l’artefact CI. Une CI PASS ne vaut ni signature, ni installation, ni réception instrumentale.

## Paquet et installation
1. Télécharger l’artefact `XoneEngineNative-V0.2-<SHA>` d’un job iOS PASS. Conserver COMMIT.txt, IPA_SHA256.txt et le lien du run. Vérifier le SHA256 de l’IPA. L’IPA fournie est non signée, pas directement installable.
2. Sur Mac/Xcode, extraire XONE_V0_SOURCES.zip du même artefact ; `brew install xcodegen`, puis `xcodegen generate`. Ouvrir XoneEngineTest.xcodeproj.
3. Choisir sa propre équipe Apple pour les deux cibles XoneEngineTest et XoneMidiAU. Les IDs source sont org.xonelab.XoneEngineNativeV02 et son extension .XoneMidiAU ; si des IDs uniques sont nécessaires, modifier project.yml puis régénérer, en gardant le préfixe commun. Ne pas modifier aumi/XnLb/Xn02.
4. Connecter l’appareil, accepter la confiance et le mode développeur si demandé. Run sur cet iPhone/iPad, ouvrir le conteneur puis ajouter le MIDI Processor XONE dans AUM. Conserver le journal de signature et le SHA. Aucun certificat, UDID ou profil n’est fourni par la CI.

Le dossier recette contient ce guide, K2_SIM_V3.txt et son README. Le ZIP source contient également le tableau docs/reprise/RESULTATS_AUM_V0.csv et les tests natifs.

## Routage et commandes d’interface
Créer une session isolée XONE_V0_<SHA>_RECETTE : **K2 ou K2_SIM**, jamais les deux, vers XONE ; sortie XONE vers moniteur POST. Ajouter éventuellement un moniteur PRE parallèle à l’entrée. Aucun Thru, retour sortie→XONE ou instrument dans cette recette.

MUSIC utilise CH7..14 ; les LED utilisent exclusivement CH15. Pour le K2 physique, envoyer CH15 de XONE seulement à son entrée de feedback. Le moniteur POST peut afficher les deux flux, mais les verdicts MUSIC doivent exclure les LED. Une instance neuve purge les100 bindings LED connus ; elle n’émet pas de RESET musical.

L’interface AU propose **Éditeur PRESET** et **Rafraîchir les LED**. Les mêmes contrôles existent dans l’arbre des paramètres du host. Le rafraîchissement ne change ni LIVE, ni les banques, ni le transport. Le panneau Diagnostic conserve les traces ; **Charger la simulation de test** est facultatif, remplace le LIVE après vrais UP/OFF dus et n’est jamais appelé automatiquement. Ne pas l’utiliser pour le parcours sans fixture ci-dessous.

K2_SIM simule les notes/CC physiques CH15, sans décision musicale : pads0..3 BOT1..4 ; knob7 choisit la page des12 pads suivants. Les notes sont celles indiquées par ses étiquettes. Les knobs0..5 couvrent CC0..5, puis SHIFT GUI CC6..11, CC12..17, CC18..21. Les relâchements restent ceux du vrai pad DOWN même si sa page change. Compilation Mozaic à vérifier sur l’appareil.

## Parcours principal sans fixture
Transport120 BPM,4/4 en marche. DOWN/UP désigne une note MIDI CH15 et son vrai relâchement ; un tap est court, sauf indication contraire. Ne pas remplacer une session CURRENT. Relever PASS/FAIL/NON_TESTABLE dans RESULTATS_AUM_V0.csv.

| ID | Action | Résultat observable attendu |
|---|---|---|
| E00 | Instance neuve, sans simulation ; ouvrir/fermer l’interface | Pas de sortie MUSIC de boot ; LED CH15 uniquement. Diagnostic et commandes disponibles, pas de crash. Les valeurs non reçues restent UNKNOWN. |
| E01 | Tap note24 (INST1 page CUE), puis25 (CUE4/BASE) | Commande propriétaire INST1 appliquée au prochain BAR, pas au BEAT intermédiaire. CC934 : `BD 26 7F`, puis97 valeurs nominales locales. Aucun quatrième PATCH, aucun programme externe. Mix et autres owners inchangés. |
| E02 | Tap36 (INST1 page1), DOWN48 puis vrai UP après220ms | SMART local désormais connu via BASE : `99 00 7F`, puis `89 00 00`. Un tap court latche ; un second tap éteint. Aucune destination physique supposée chargée. |
| E03 | Tap37 pour capturer MEMORY1 ; CC4=0 puis100 ; tap37 | FX0 devient100 (`BB 4B 64`) après pickup ; rappel des9 valeurs au prochain BEAT, retour à0 pour ce champ. Source et autres MEMORY préservées. |
| E04 | DOWN13, DOWN15, UP15, UP13 pour MACHINE ; DOWN14, tap52, UP14 pour SAVE PATCH1 ; CC4=0 puis73 ; tap52 | Paire M1/M2 de INST1 sauvegardée60 valeurs. Modification MACHINE2 : `B7 12 49`. Rappel PATCH1 au BAR : `B7 12 00`. MEMORY de cette source désengagée, banque non effacée ; modes S8 et autres sources préservés. |
| E05 | Refaire13+15 pour quitter MACHINE. Activer Éditeur PRESET ; tap36 pour capturer PRESET1 ; CC16=64 puis100 ; tap12, CC16=77, tap12 | CC16 est maintenant global : `BA 30 40`, `BA 30 64`, `BA 30 4D`, puis RED restaure100 immédiatement (`BA 30 64`). RED ne demande aucun programme externe. Attention : en sous-contexte MACHINE, CC16 possède une autre cible ; ne pas les confondre. |
| E06 | Tap36 sur PRESET puis attendre BAR | Restaure le monde capturé. Si aucun CC16 n’avait été reçu avant la capture, ce champ reste UNKNOWN et aucun zéro de remplacement n’est émis. Sinon sa vraie valeur capturée est rappelée. Pas de restauration des modes S8. |
| E07 | Désactiver Éditeur PRESET. BOT2 court13 pour MUTE ; BASE36 court puis BASE37 long≥200ms | MUTE et SOLO appliqués au BAR. MUTE source1 `9A 10 7F` ; états/LED reflètent l’état appliqué et clignotent pendant l’attente. Quitter la page puis vérifier la vue sous-jacente. Aucun effet audio sans liaison AUM. |
| E08 | Armer un rappel puis STOP avant quantification ; faire aussi STOP avec48 tenu après ON, puis vrai UP | Pending annulés, OFF dû livré, UP tardif sans nouvel ON. Vérifier transport AUM et, si disponible, STOP MIDI FC. Aucun UP fabriqué. Refaire START : aucune ancienne action rejouée. |

Les modes SPOT SAMPLE/MACHINE restent UNKNOWN tant qu’ils ne sont pas explicitement configurés ; un SPOT exigeant ce mode doit être refusé, pas converti implicitement en MIX. Le SMART E02 relève du SOCLE local connu après BASE, pas d’un mode S8 inventé. Les bindings et références BASE des alternatives sont des configurations natives explicites ; la recette ne choisit aucun synthé ni preset externe. Les éditeurs de profils S8 ne sont pas exposés par cette interface minimale.

## Sauvegarde et reprise
| ID | Préparation / action | Vérification |
|---|---|---|
| E09 | Revenir à INST1 page1 ; capturer une MEMORY engagée connue, modifier son FX sans la rappeler. STOP, vrais UP et OFF constatés ; sauvegarder la session AUM, fermer/réouvrir cette même session | V2 reprend LIVE/NAV, banques PATCH/MEMORY/PRESET et configurations autorisées. Relancer le transport120BPM avant le tap de la MEMORY encore engagée : ses anciennes9 valeurs reviennent au BEAT, et non les valeurs éditées après capture. Rappeler aussi le PATCH sauvé pour vérifier sa banque. Les observations externes restent UNKNOWN. |
| E10 | Observer le reopen, puis PRESET/RED et STOP | Projection des seules valeurs nominales connues, dont les16 états MUTE ; pas de TOP/GROUP/LONG, SOFT ou LOAD/CUE historique. RED actif et pending ne sont pas ressuscités. Un ancien document LIVE V1 reste lisible. Les corruptions de fichier sont vérifiées nativement, sans modifier une session AUM réelle. |
| E11 | Reconnecter K2 puis appuyer Rafraîchir les LED ; changer de page et actionner un knob CC4..15 | Purge OFF avant ON sur les34 positions ; BOT1 rouge seulement. Flash pickup100ms selon mode connu ; si même couleur déjà allumée, extinction temporaire puis retour. UNKNOWN n’allume pas une couleur inventée. Vérifier matériellement les couleurs, pas seulement les octets POST. |
| E12 | Fermer/réouvrir seulement l’interface, puis STOP/OFF et recharger le node | Pas de crash ; l’interface ne détruit pas le moteur. Le chemin de réallocation conserve et réessaie les OFF dus au prochain rendu. Une suppression définitive sans prochain rendu ne prouve pas la livraison externe d’un OFF : constater les OFF avant retrait. |

## Contrôles et charge
- Après STOP et vrais UP, retirer entrée→XONE : aucune nouvelle commande MUSIC issue du contrôleur. Retirer XONE→POST : POST ne reçoit plus sa musique. SENT résultat0 dans la trace confirme uniquement le callback ; le moniteur POST doit confirmer le routage. Aucun ACK instrumental n’est déduit.
- Entrée hors CH15 ou câble non prévu : pas d’action physique. Pas de double entrée K2/K2_SIM ni bouclage LED vers XONE. Répéter avec seek, changement de tempo, perte puis retour du contexte hôte, deux sources armées au même BAR et interface fermée.
- Pour la charge, noter modèle, iOS, AUM, fréquence/buffer, CPU repos/pic, durée, xruns/crash et paquets POST. Les stress natifs mesurent4096 buffers128frames et l’absence d’allocations C++ du chemin testé ; ces nombres ne certifient pas le CPU ou les délais temps réel d’un iPhone.
- Si dropped>0, la trace est incomplète : isoler le cas et ralentir. Ne pas transformer une trace partielle en PASS musical.

## Portée et compte rendu
Les blocs1..4 disposent de preuves natives et CI ; la campagne AUM, la signature/installation, la compilation Mozaic, le feedback K2 physique et les mesures sur appareil sont **NON_TESTES** jusqu’au relevé utilisateur. Instruments, MIDI Learn, ports/profils matériels et presets externes restent à configurer ; absence de configuration ne doit pas être présentée comme un instrument chargé.

Consigner SHA COMMIT.txt, run CI, appareil/iOS/AUM/Mozaic, signature, routes, fréquence/buffer, verdict par ID, octets PRE/POST, trace et CPU/xruns. Ne clôturer le Bloc5 réel qu’après cette campagne. Le tableau CSV du ZIP source est un modèle vierge NON_TESTE, pas une preuve de session .aum ; aucun fichier .aum n’est fabriqué ici.
