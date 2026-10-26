# Recette AUM iPhone V0.2 — sans K2

État initial : **AUM_EN_ATTENTE / K2_EN_ATTENTE**. La CI produit un IPA non signé ; il faut le signer avec un profil iOS légitime conservant l'extension AUv3 embarquée, installer sur l'iPhone puis ouvrir l'application conteneur une fois. Aucun certificat ni réussite d'installation n'est supposé.

La V0.2 utilise `org.xonelab.XoneEngineNativeV02` et extension `.XoneMidiAU`, composant `aumi/XnLb/Xn02` ; V0.1 garde `org.xonelab.XoneEngineTest`, `Xn01`. Un outil de signature qui change ces identifiants doit conserver la cohérence app/extension et la distinction V0.1. Réutiliser la procédure de signature V0.1 déjà retenue, après vérification de sa prise en charge des extensions. Le paquet unsigned seul n'est pas installable directement par iOS.

1. Créer/copier la session de test existante à quatre tranches MIDI : TX SIM, PRE LOG, XONE ENGINE **V0.2**, POST LOG. Préserver la session V0.1 originale.
2. Matrice unique : TX→PRE ; TX→V0.2 ; V0.2 TEST OUT→POST. PRE et POST n'émettent rien. Aucune route TX→POST, sortie→entrée, instrument ou matériel.
3. Utiliser le script TX du dossier `TEST_AUM_IPHONE_V0_2_TRACE` (pads CC4 et CC8 sur CH1). Un autre émetteur virtuel est acceptable s'il produit les mêmes bytes.
4. Ouvrir l'interface V0.2. Pour `B0 04 7F`, vérifier RX_IN, DECISION MAPPED `B0 4A 7F`, TX_OUT SENT résultat0 et un seul message POST CC74=127 CH1. Pour `B0 08 7F` attendre `B0 47 7F`. Refaire valeur0, puis64.
5. Les numéros `sequence` relient RX/DECISION/TX ; `s` est la position échantillon hôte, pas un compteur BAR inventé. Un TX_OUT avec erreur n'est pas un envoi réussi. `NO_OUTPUT` signifie callback de sortie absent. `dropped>0` invalide l'exhaustivité de la capture ; ralentir puis recommencer.
6. CC74/71 renvoyés en entrée ne produisent aucun TX ; CC9, notes, PC, SysEx ou CH2 sont refusés. CH15 : notes36 DOWN / NoteOn vélocité0 UP traversent SurfaceInput en diagnostic seulement, aucune sortie musicale.
7. Arrêter/reprendre le transport AUM : aucun MIDI fabriqué, aucun faux UP. Les CC CH1 restent immédiats et indépendants du transport ; aucune fonction BAR/BEAT n'est certifiée par cette recette.
8. Déconnecter V0.2→POST : POST doit se taire. Déconnecter TX→V0.2 : aucune transformation. Ces contrôles négatifs détectent un contournement dans la matrice.
9. `RX_FEEDBACK=UNKNOWN` reste normal. Aucun câble AUM n'authentifie un instrument ; aucune comparaison TX/valeur ne crée MATCH. Un éventuel câble non0 apparaît UNTRUSTED sans effet.
10. Fermer/réouvrir l'interface et charger V0.1 séparément pour vérifier coexistence ; la sauvegarde d'un LIVE musical AUM reste L13, pas une promesse de cette version.

Preuves à conserver : SHA du fichier COMMIT de l'artefact, SHA IPA, signature/installation, modèle et iOS/AUM, capture matrice, RX/DECISION/TX et POST, erreurs/compteurs. Verdict par cas, AUM PASS uniquement après ces observations. Le K2 reste EN_ATTENTE indépendamment de cette recette.
