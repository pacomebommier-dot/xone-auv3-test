# Six destinations SYNTH dans AUM

Décisions utilisateur du 6 novembre 2026. Le sélecteur PRESET existe dans le build e74f0a6. Le routage sélectif et les échanges décrits ici restent à raccorder.

## Affectations

| Slot XONE | Destinations autorisées | Session neuve |
|---|---|---|
| INST1 | SYNTH 1–4 | SYNTH 1 |
| INST2 | SYNTH 1–4 | SYNTH 2 |
| YELLOW1 | SYNTH 3–6 | SYNTH 5 |
| YELLOW2 | SYNTH 3–6 | SYNTH 6 |

Les numéros désignent les six tranches audio AUM du bus SYNTH. SYNTH 1 reçoit le SH‑01A. Les cinq autres identités de plugin restent à définir. SYNTH 3 et 4 sont libres en session neuve. Une session enregistrée retrouve ses affectations.

Le sélecteur PRESET existant conserve ses quatre slots logiques et quatre choix par slot. Les deux rangées désignent successivement le slot puis sa destination. SYNTH 3 et 4 ont une identité unique, accessible des deux familles. Une destination active possède au plus un slot XONE.

## Sélection et échange

- Destination libre. Le demandeur change de destination. L’ancienne devient libre et conserve ses données. Revenir vers elle est une nouvelle sélection libre.
- Destination occupée. Les deux slots échangent leur destination lorsque chacun accepte sa destination d’arrivée. Exemple valide, INST2 demande le SH déjà sur INST1 et INST1 prend l’ancien SYNTH d’INST2.
- Échange impossible. Si l’une des destinations n’est pas autorisée dans l’autre slot, refuser la demande entière. Exemple, INST1 sur SYNTH 1 demande SYNTH 3 pris par YELLOW1. YELLOW1 ne peut recevoir SYNTH 1.
- Garder la frontière BAR ou transport explicitement arrêté déjà employée par le moteur. Clore les NoteOff dus sur les anciennes destinations avant publication de l’affectation. Un échec ne laisse aucune moitié d’échange appliquée.

## Données et routage

Dans une session, la dernière image courante et les données associées au synthé suivent son identité, y compris lorsqu’il n’occupe plus de slot. Les mémoires et les contenus des PRESET qui le concernent retrouvent ce même synthé après déplacement. Les autres domaines globaux et les bus restent distincts. La première activation sans image courante utilise le BASE disponible. Une réactivation reprend l’image courante conservée.

Les réglages MACHINE, les PATCH et le BASE appartiennent à l’identité du synthé d’une session à l’autre. Les réglages de jeu, MEMORY et PRESET de session restent dans leur session et suivent le synthé à l’intérieur de celle-ci. Pour le SH, WRITE physique et miroirs automatiques PC1–3 restent régis par le contrat SH36. Séparer les observations matérielles des états de session.

La grille AUM comporte six destinations stables. Le routage des commandes liées au synthé suit la destination active et garde les commandes de bus et LED dans leurs domaines. Le raccordement n’envoie pas la même commande aux six pistes. Le document AU doit préserver l’identité du synthé dans les affectations et les données qui le suivent. Les anciens documents restent lisibles. Toute équivalence d’anciens identifiants entre les deux pools doit être établie avant migration.

## Voyants de la dernière rangée PRESET

Pour le slot consulté, vert fixe désigne sa destination active. Vert clignotant désigne sa sélection en attente. Jaune désigne une destination occupée avec laquelle l’échange est admissible. Rouge désigne une destination occupée dont l’échange est refusé. Une destination libre reste éteinte.

Le premier correctif restaure les valeurs MACHINE connues du dernier LIVE déjà enregistré par la banque existante. Il conserve le BASE pour une première activation et pour un rappel explicite. Le rouge signale les conflits déjà refusés par l’arbitre actuel. L’indication jaune d’un échange admissible attend la mise en œuvre des échanges entre les six destinations.

## Câbles AUv3

La sortie existante conserve le numéro 0. Les sorties SYNTH 1 à 6 utilisent les câbles virtuels 1 à 6. Une commande nominale appartenant à une alternative active emprunte son câble SYNTH. Les octets S7 conservent leur adresse logique dans ce raccordement. Les bus, les LED et le trafic de l’adaptateur SH historique conservent la sortie commune. Un NoteOff de bascule est envoyé avant la confirmation de la nouvelle affectation.

Les nouvelles sélections et les rappels globaux refusent les doubles destinations sur un même câble. Un ancien document comportant un tel chevauchement reste lisible et utilise la sortie historique pour cette ouverture. L’échange, l’activation des affectations de démarrage et la traduction indépendante du slot jusqu’au plugin restent à raccorder. La matrice AUM doit encore être validée sur iPad avec les sorties nommées.

## Cas à vérifier

1. Session neuve avec la grille par défaut et SYNTH 3/4 libres.
2. INST1 sur SYNTH 1 passe à SYNTH 3 libre, puis revient à SYNTH 1. Les réglages différents des deux synthés sont retrouvés.
3. INST2 prend le SH déjà sur INST1. Les deux affectations échangent, les réglages et mémoires suivent, les anciens OFF sortent sur l’ancienne destination.
4. Un échange entre familles sur SYNTH 3/4 est accepté seulement si les deux arrivées sont admissibles. Une arrivée interdite laisse les deux affectations et états intacts.
5. Enregistrer et rouvrir la même session après l’échange. Vérifier affectations, LIVE, BASE, PATCH, MEMORY et PRESET, puis refaire une sélection libre. Vérifier séparément le retour du SH dans une autre session.
6. Dans AUM, vérifier les six routes séparément avec un moniteur, puis le SH physique. Une commande nominale ne doit atteindre que sa destination. Le comportement matériel reste à constater sur iPad.

Le build e74f0a6 utilise deux pools indépendants et des adresses virtuelles indexées par slot XONE. Une grille AUM à six pistes ne suffit donc pas à fournir ce comportement. Le chantier doit raccorder l’arbitrage, l’état persistant et l’émission avant de qualifier la session AUM.
