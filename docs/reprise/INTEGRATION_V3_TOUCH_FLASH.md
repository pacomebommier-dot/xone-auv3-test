# TOUCH / FLASH dans le moteur AUv3 — 27 octobre 2026

## Source et raccordement
Suite de525bb80 sur feature/native-integration-v3. Références : L10 S3 @zs/@w0/@w1/@w3, S4 @OutIntent/@OutOn/@OutOff et contrôle des propriétaires dans @RawReceive. Les12 headers Core et14 suites historiques restent inchangés.

Le mode connu3 (TOUCH) et4 (FLASH), pour une vraie route SPOT d6=3, est désormais consommé par la même chaîne K2 -> S1/S2/S3 -> LIVE -> S4 -> callback AU. Pas de défaut de mode ou d’état synthétisé. Les routes d6=24 restent exclues par @w0. Les modes1/2 existants sont conservés.

## TOUCH
Depuis un état0 connu, le DOWN capture la destination et utilise l’intention S4 type12 avec sa touche propriétaire. La confirmation du callback conserve la dette et l’owner ; un ON en erreur reste une émission possible. Le UP met le nominal à0 puis libère exclusivement le propriétaire capturé via type13. Un OFF échoué ne libère ni owner ni dette. Depuis un état1 connu, le parcours nominal ON puis OFF reste appliqué conformément à @w0/@w1. Aucun seuil SMART ne transforme TOUCH en latch.

## FLASH
Le DOWN remet LIVE à0 et projette le OFF nominal préalable avec S4. Le type14 source projette ensuite ON/OFF. Cette séquence tient dans les3 paquets déjà prévus par Result ; chaque paquet conserve ses propres métadonnées.
Si le OFF préalable échoue ou si le callback est absent, le ON du pulse est refusé. Un ON de pulse échoué reste suivi d’un OFF, car il peut avoir partiellement atteint la sortie. Un OFF de pulse échoué reste dû au STOP suivant. Le UP physique FLASH reste inerte ; chaque nouvel appui produit un nouveau pulse, sans déduplication d’action.
Pour conserver une dette après l’échec du second callback d’un pulse, l’adaptateur confirme ses deux moitiés avec les opérations S4 owner12/release13 ; il ne marque pas le type14 entier comme envoyé après le seul ON. Le nominal reste0 pendant le pulse.

## STOP, callback et états
Le wrapper appelle canSend immédiatement avant chaque callback. Le OFF FLASH préalable doit être confirmé pour cette même séquence et adresse. Le compteur de séquence existant invalide les ON préparés avant STOP ou un autre événement ; les OFF déjà dus restent autorisés. Aucun protocole ACK/retry universel ni nouvel ordonnanceur n’est ajouté.
STOP ferme les dettes en conservant leurs owners jusqu’à confirmation réelle des OFF. Les touches restent Held jusqu’au vrai UP ; aucun redéclenchement fantôme. TOUCH revient au désiré0 au STOP. À épuisement du compteur LIVE, un OFF dû peut être émis sans inventer un commit LIVE refusé. Une restauration de session reste différée tant qu’une note est due, même après le vrai UP.

## Vérification et limites
Tests/test_host_touch_flash.cpp : états initiaux0/1, bytes MIDI exacts, répétition FLASH, DOWN dupliqué, callback absent/échoué à chaque phase, STOP, annulation d’un ON non envoyé, owner conservé, restauration différée, borne de révision et UNKNOWN. Le harness de la vraie classe AU exerce TOUCH/FLASH et les échecs de callback dans la boucle de rendu sous macOS.
La fixture MACHINE connue est une référence logicielle de test. Ce raccordement ne prouve ni port authentifié, ni destination S8 réelle, ni état Roland observé. Les registres de propriétaires S8 par destination, actions BAR/BEAT, banques et session AUM restent ouverts. Aucun test iPhone, AUM natif, K2 ou SH-01A nouveau ; aucune clôture matérielle L04/L06/L15/L16.

Validation locale :22 suites C++ GCC16.2 PASS,10 cas C V0.1 PASS ; manifest12/14 et contrôles source K2_SIM/SH-01A PASS. Preuve : evidence/v3/TOUCH_FLASH_LOCAL_GCC.json. Validation Apple et iOS à confirmer par la CI du nouveau commit.

Ordre des événements corrigé après test RED : la capture de route SPOT se fait après la qualification S2/S3 de cet événement, car un LONG peut devenir valide à son timestamp. Son UP conserve cette capture même après retour du NAV temporaire. Le harness AU couvre un appui situé480frames après le début d’un buffer48kHz qui franchit ce seuil ; les suites historiques restent inchangées.

La CI812b5b3 conserve un échec du nouveau test AU intra-buffer (22 suites et sanitizers PASS, émission attendue absente). Le harness réinstalle désormais son bloc callback conservé explicitement dans chaque nouvelle AU, au lieu de relire le callback d’une AU dont les ressources ont été libérées. Le test d’émission reste inchangé, avec trace conditionnelle en cas d’échec. La CI suivante doit confirmer ce diagnostic.

CI finale PASS sur9e2866911331bf70e1aa439dbc62ef9f415aadf5 : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40478692311 . Les22 suites,10 cas C, sanitizers, vraie classe AU macOS, build iOS et archivage sont PASS. Le diagnostic du harness confirme : callback conservé=1, callback sur ancienne AU désallouée=0. L’assertion de MIDI attendu a été conservée et passe, y compris le franchissement LONG intra-buffer. Preuves : evidence/v3/CI_9E28669.json ; le digest fourni concerne le ZIP GitHub, pas directement l’IPA. Aucun nouveau PASS AUM ou matériel.
