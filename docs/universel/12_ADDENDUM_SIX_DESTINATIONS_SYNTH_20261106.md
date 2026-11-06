# Six destinations SYNTH dans AUM

DÃ©cisions utilisateur du 6 novembre 2026. Le sÃ©lecteur PRESET existe dans le build e74f0a6. Le routage sÃ©lectif et les Ã©changes dÃ©crits ici restent Ã  raccorder.

## Affectations

| Slot XONE | Destinations autorisÃ©es | Session neuve |
|---|---|---|
| INST1 | SYNTH 1â€“4 | SYNTH 1 |
| INST2 | SYNTH 1â€“4 | SYNTH 2 |
| YELLOW1 | SYNTH 3â€“6 | SYNTH 5 |
| YELLOW2 | SYNTH 3â€“6 | SYNTH 6 |

Les numÃ©ros dÃ©signent les six tranches audio AUM du bus SYNTH. SYNTH 1 reÃ§oit le SHâ€‘01A. Les cinq autres identitÃ©s de plugin restent Ã  dÃ©finir. SYNTH 3 et 4 sont libres en session neuve. Une session enregistrÃ©e retrouve ses affectations.

Le sÃ©lecteur PRESET existant conserve ses quatre slots logiques et quatre choix par slot. Les deux rangÃ©es dÃ©signent successivement le slot puis sa destination. SYNTH 3 et 4 ont une identitÃ© unique, accessible des deux familles. Une destination active possÃ¨de au plus un slot XONE.

## SÃ©lection et Ã©change

- Destination libre. Le demandeur change de destination. Lâ€™ancienne devient libre et conserve ses donnÃ©es. Revenir vers elle est une nouvelle sÃ©lection libre.
- Destination occupÃ©e. Les deux slots Ã©changent leur destination lorsque chacun accepte sa destination dâ€™arrivÃ©e. Exemple valide, INST2 demande le SH dÃ©jÃ  sur INST1 et INST1 prend lâ€™ancien SYNTH dâ€™INST2.
- Ã‰change impossible. Si lâ€™une des destinations nâ€™est pas autorisÃ©e dans lâ€™autre slot, refuser la demande entiÃ¨re. Exemple, INST1 sur SYNTH 1 demande SYNTH 3 pris par YELLOW1. YELLOW1 ne peut recevoir SYNTH 1.
- Garder la frontiÃ¨re BAR ou transport explicitement arrÃªtÃ© dÃ©jÃ  employÃ©e par le moteur. Clore les NoteOff dus sur les anciennes destinations avant publication de lâ€™affectation. Un Ã©chec ne laisse aucune moitiÃ© dâ€™Ã©change appliquÃ©e.

## DonnÃ©es et routage

Dans une session, la derniÃ¨re image courante et les donnÃ©es associÃ©es au synthÃ© suivent son identitÃ©, y compris lorsquâ€™il nâ€™occupe plus de slot. Les mÃ©moires et les contenus des PRESET qui le concernent retrouvent ce mÃªme synthÃ© aprÃ¨s dÃ©placement. Les autres domaines globaux et les bus restent distincts. La premiÃ¨re activation sans image courante utilise le BASE disponible. Une rÃ©activation reprend lâ€™image courante conservÃ©e.

Les rÃ©glages MACHINE, les PATCH et le BASE appartiennent Ã  lâ€™identitÃ© du synthÃ© dâ€™une session Ã  lâ€™autre. Les rÃ©glages de jeu, MEMORY et PRESET de session restent dans leur session et suivent le synthÃ© Ã  lâ€™intÃ©rieur de celle-ci. Pour le SH, WRITE physique et miroirs automatiques PC1â€“3 restent rÃ©gis par le contrat SH36. SÃ©parer les observations matÃ©rielles des Ã©tats de session.

La grille AUM comporte six destinations stables. Le routage des commandes liÃ©es au synthÃ© suit la destination active et garde les commandes de bus et LED dans leurs domaines. Le raccordement nâ€™envoie pas la mÃªme commande aux six pistes. Le document AU doit prÃ©server lâ€™identitÃ© du synthÃ© dans les affectations et les donnÃ©es qui le suivent. Les anciens documents restent lisibles. Toute Ã©quivalence dâ€™anciens identifiants entre les deux pools doit Ãªtre Ã©tablie avant migration.

## Voyants de la derniÃ¨re rangÃ©e PRESET

Pour le slot consultÃ©, vert fixe dÃ©signe sa destination active. Vert clignotant dÃ©signe sa sÃ©lection en attente. Jaune dÃ©signe une destination occupÃ©e avec laquelle lâ€™Ã©change est admissible. Rouge dÃ©signe une destination occupÃ©e dont lâ€™Ã©change est refusÃ©. Une destination libre reste Ã©teinte.

Le premier correctif restaure les valeurs MACHINE connues du dernier LIVE dÃ©jÃ  enregistrÃ© par la banque existante. Il conserve le BASE pour une premiÃ¨re activation et pour un rappel explicite. Le rouge signale les conflits dÃ©jÃ  refusÃ©s par lâ€™arbitre actuel. Lâ€™indication jaune dâ€™un Ã©change admissible attend la mise en Å“uvre des Ã©changes entre les six destinations.

## CÃ¢bles AUv3

La sortie existante conserve le numÃ©ro 0. Les sorties SYNTH 1 Ã  6 utilisent les cÃ¢bles virtuels 1 Ã  6. Une commande nominale appartenant Ã  une alternative active emprunte son cÃ¢ble SYNTH. Les octets S7 conservent leur adresse logique dans ce raccordement. Les bus, les LED et le trafic de lâ€™adaptateur SH historique conservent la sortie commune. Un NoteOff de bascule est envoyÃ© avant la confirmation de la nouvelle affectation.

Les nouvelles sÃ©lections et les rappels globaux refusent les doubles destinations sur un mÃªme cÃ¢ble. Un ancien document comportant un tel chevauchement reste lisible et utilise la sortie historique pour cette ouverture. Lâ€™Ã©change, lâ€™activation des affectations de dÃ©marrage et la traduction indÃ©pendante du slot jusquâ€™au plugin restent Ã  raccorder. La matrice AUM doit encore Ãªtre validÃ©e sur iPad avec les sorties nommÃ©es.

## Cas Ã  vÃ©rifier

1. Session neuve avec la grille par dÃ©faut et SYNTH 3/4 libres.
2. INST1 sur SYNTH 1 passe Ã  SYNTH 3 libre, puis revient Ã  SYNTH 1. Les rÃ©glages diffÃ©rents des deux synthÃ©s sont retrouvÃ©s.
3. INST2 prend le SH dÃ©jÃ  sur INST1. Les deux affectations Ã©changent, les rÃ©glages et mÃ©moires suivent, les anciens OFF sortent sur lâ€™ancienne destination.
4. Un Ã©change entre familles sur SYNTH 3/4 est acceptÃ© seulement si les deux arrivÃ©es sont admissibles. Une arrivÃ©e interdite laisse les deux affectations et Ã©tats intacts.
5. Enregistrer et rouvrir la mÃªme session aprÃ¨s lâ€™Ã©change. VÃ©rifier affectations, LIVE, BASE, PATCH, MEMORY et PRESET, puis refaire une sÃ©lection libre. VÃ©rifier sÃ©parÃ©ment le retour du SH dans une autre session.
6. Dans AUM, vÃ©rifier les six routes sÃ©parÃ©ment avec un moniteur, puis le SH physique. Une commande nominale ne doit atteindre que sa destination. Le comportement matÃ©riel reste Ã  constater sur iPad.

Le build e74f0a6 utilise deux pools indÃ©pendants et des adresses virtuelles indexÃ©es par slot XONE. Une grille AUM Ã  six pistes ne suffit donc pas Ã  fournir ce comportement. Le chantier doit raccorder lâ€™arbitrage, lâ€™Ã©tat persistant et lâ€™Ã©mission avant de qualifier la session AUM.
