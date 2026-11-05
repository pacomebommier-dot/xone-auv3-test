# BASE local, CUE4 et DRUMS/FILL 7–8

Décisions du 5 novembre 2026. Contrat actif pour le prochain patch AUv3. Implémentation à réaliser.

## Références et ouverture

Pour chaque paramètre local admissible, le BASE effectif est la référence personnalisée de la destination active si elle existe, sinon la référence universelle. L’absence de plugin n’empêche aucune commande neutre admissible. La provenance des valeurs universelles est le GOLDEN et les décisions explicites plus récentes. Les adresses restent celles de S7.

À l’ouverture, prendre le LIVE sauvegardé pour les champs présents. Compléter les valeurs locales manquantes avec le BASE effectif. En session neuve, le LIVE de départ vient du BASE. Conserver l’image de départ de la session pour les rappels MEMORY groupés dont un slot est vide. Restaurer les attributions et les banques. Initialiser les références sans émission MIDI de démarrage ni rappel automatique du BASE. Les règles de projection déjà applicables au LIVE restauré restent distinctes de cette initialisation.

RESET applique le BASE effectif de son périmètre. SAVE MEMORY capture le LIVE courant. Un rappel groupé sur un slot vide utilise le LIVE d’ouverture, même s’il diffère du BASE. Un slot vide reste vide. Une observation externe absente, une adresse réservée, un service indisponible ou un mode hors domaine conservent leur statut propre.

| Paramètres | BASE universel |
|---|---|
| Rotatifs des six sons de DRUMS et des six sons FILL | 0, par initialisation VLOUT du GOLDEN et routes BICC6/BICC11 |
| EQ du socle INST et YELLOW | 63, exceptions GOLDEN BICC1/BICC8 |
| Six EQ BUS3/TOTAL de la page commune | 63, décision du 5 novembre |

Relever les autres références champ par champ dans les sources applicables avant de les activer. Conserver les états OFF et modes MIX dans leur domaine et leurs règles S8. Un zéro de la table brute ne transforme pas une adresse non affectée en commande.

## CUE4 et BASE de tranche

CUE/LOAD 1 à 3 restent les trois PATCH. CUE4 désigne le quatrième choix de la page CUE/LOAD d’INST1, INST2, YELLOW1 ou YELLOW2. TOP4 reste FILTER GLOBAL. Les quatre PRESET globaux gardent leurs quatre emplacements.

- Court sur CUE4. Au vrai relâchement, armer le rappel BASE pour la tranche et la destination capturées au DOWN. Utiliser le BAR ou l’application à l’arrêt déjà prévue.
- Long sur CUE4. Au vrai relâchement, enregistrer une seule fois le BASE complet du propriétaire capturé. Le clignotement confirme l’écriture réussie. BASE actif prend la couleur de sous-page et BASE altéré la couleur ALT. Le statut compare les valeurs du périmètre au BASE de la destination engagée. Les palettes exactes doivent être vérifiées contre le GOLDEN.
- Le BASE contient les valeurs MACHINE autorisées et tous les FX INST de la tranche. Les alias des MACHINE paires partagent leurs cellules FX. Les modes de configuration restent hors capture.
- Une nouvelle attribution applique le BASE de la destination. Un rappel CUE4 applique le même BASE. La réouverture AUM reprend le LIVE sauvegardé et retrouve la référence BASE pour les actions suivantes.
- SH-01A. Le programme 1 en façade est le BASE pour chaque destination SH configurée. CUE4 court envoie PC0 et applique les FX locaux. Un rappel du programme 1 depuis le Roland applique les mêmes FX BASE dans XONE et actualise le LIVE SH par les CC reçus. CUE4 long enregistre les FX locaux et le feedback confirme cette écriture XONE. Le son du programme 1 se sauvegarde par WRITE physique sur le Roland. CUE1–3 rappellent les programmes 2–4 par PC1–3. Les trois miroirs restent automatiques et les FX restent hors des PATCH SH.

## DRUMS et FILL

Chacune des deux familles possède six sons. Chaque son dispose de quatre appels SOFT externes et de trois MEMORY locales.

| Action | DRUM7 / FILL7 | DRUM8 et FILL8, page GLOBAL commune |
|---|---|---|
| Rotatifs | Paramètre homologue des six sons de la famille. Mouvement sans saut. Convergence à 0 ou 127 à la butée, puis reprise à valeur commune. | Trois EQ du socle BUS3 puis trois EQ du socle TOTAL. Une valeur, une adresse et un pickup par cible, partagés avec GLOBAL. |
| SPOT | RESET du seul paramètre homologue des six sons, chacun à son propre BASE. | RESET du seul EQ du rotatif associé. |
| SOFT 1 à 4 | Même numéro de SOFT externe sur les six sons de la famille. | Même numéro sur les douze sons DRUMS et FILL. |
| SAVE MEMORY 1 à 3 | Capturer le même numéro de slot individuel sur les six sons. | Capturer le même numéro de slot individuel sur les douze sons. |
| Rappel MEMORY 1 à 3 | Pour chaque son, appliquer le slot existant ou le LIVE d’ouverture si vide. | Même règle sur les douze sons. |

Pour MEMORY DRUMS/FILL, sur un son ou sur un groupe, l’appui court garde la logique du GOLDEN. Un slot inactif est capturé et engagé. Un slot engagé est rappelé au BEAT. Pour le groupe, un seul slot engagé suffit à choisir le rappel. Les membres dont le slot est vide reprennent leur LIVE d’ouverture. Un appui long de 200 ms ou plus sur un slot engagé remplace la MEMORY du son ou les MEMORY du groupe par les valeurs courantes. Sur un slot inactif, court et long font la même première capture. Les deux gestes sont exclusifs et se déclenchent au vrai relâchement. STOP annule un appui encore tenu. Les couleurs groupées retenues pour la recette sont éteint pour vide, jaune pour mixte, rouge pour complet et vert clignotant pour rappel en attente. Ces choix de surface restent à valider sur K2.

SOFT laisse les réglages locaux en place. La page 8 ne rappelle aucun BASE ni PATCH des SYNTH et ne crée pas de banque MEMORY GLOBAL. Les six EQ GLOBAL sont hors du périmètre de cette MEMORY groupée.

| Rotatifs | Cellules GLOBAL | API S7 | RESET universel |
|---|---|---|---|
| 1 à 3 | 1467–1469 | 699–701 | Canal 12, CC 59–61, valeur 63 |
| 4 à 6 | 1470–1472 | 702–704 | Canal 12, CC 62–64, valeur 63 |

## Arbitrages et contradictions résolues

| Formulation antérieure ou comportement actuel | Règle active |
|---|---|
| BASE local provisoire seulement à zéro et RESET individuel inconnu dans 00/01 et le plan initial | BASE universel documenté, surchargé par le BASE personnalisé. Les opérations locales du périmètre couvert ont une référence. |
| LIVE local systématiquement UNKNOWN en session neuve dans le bilan Bloc 2 | Compléter les valeurs locales depuis le BASE. Conserver l’observation externe inconnue jusqu’à réception. |
| CUE4 arme au DOWN et utilise un BASE MACHINE avec FX remis à zéro | Décider court ou long au vrai UP. Rappeler ou enregistrer le BASE complet avec FX. |
| Réouverture assimilée à une nouvelle attribution | Réouverture = LIVE sauvegardé. Nouvelle attribution = application du BASE. |
| DRUMS/FILL pages 7/8 traitées comme sons par les routes SOFT et MEMORY actuelles | Pages 7/8 agrègent les propriétaires 1 à 6. Les anciens enregistrements dédiés 7/8 restent des données historiques. |
| Dernière page avec rappels SYNTH ou MEMORY des six EQ | Dernière décision. SOFT et MEMORY visent les douze sons. Les six EQ sont des miroirs avec RESET individuel. |
| Aucun WRITE de PATCH SH | Règle maintenue. Le nouveau WRITE BASE côté XONE stocke seulement la référence et les FX locaux. |

Les sources et preuves historiques gardent leur contenu. Le contrat SH36 reste l’autorité des trois PATCH SH. Les règles MACHINE paires restent l’autorité des alias FX. Voir le [plan de patch AUv3](../reprise/PLAN_PATCH_BASE_DRUMS_FILL_20261105.md).
