# Profils MACHINE — état préparatoire

SH01A_SOURCE_V1.json consigne uniquement les numéros CC et capacités documentés par Roland pour le SH-01A. La liste contrôlée vient de la page1 du PDF officiel indiqué dans le fichier. Ce n'est ni le SH-01, ni un profil JU-06A.
Les valeurs d'énumération, defaults, références BASE/PC, canal et port restent absents. Un numéro CC documenté ne prouve pas que tous les octets0..127 représentent les valeurs métier attendues. CC84 n'a pas de contrôle nommé : aucune affectation déduite du simple intervalle71..87.
Le tableau distingue les contrôles reconnus et ceux émis selon la documentation ; TX/RX déclarés ne constituent pas une observation authentifiée de l'instrument réel.
Aucun émetteur MACHINE n'est raccordé. Le profil est SOURCE_VERIFIED_NOT_BOUND : pas de notes musicales SH jouées par XONE, pas de SysEx de requête inventé, pas d'activation ni d'émission BASE au chargement. Les commandes de performance HOLD/PORTAMENTO sont seulement inventoriées ; aucun binding XONE n'est accordé par cette liste.
Les effets audio de la tranche AUM restent pilotés via l'API neutre S7 et MIDI Learn, indépendamment des CC MACHINE SH. JU-06A reste une phase ultérieure et exige un profil distinct.
Validation matérielle et complétude des références : en attente L16. Préparation des correspondances XONE et tranches FX : L14 reste ouvert.
