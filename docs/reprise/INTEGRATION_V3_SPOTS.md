# Raccordement SPOT SMART/MIX — 27 octobre 2026

La chaîne AUv3 traite désormais les vrais appuis K2 SPOT (contrôles20..31), après S1/S2/S3. La route NAV est capturée avant traitement du DOWN, puis résolue avec LiveState::spotAddresses et les formules S4 source. Le UP utilise exclusivement cette capture, même si une sélection FX a changé entre-temps.

Référence : L10_S3_NONCURRENT @z3/@zs/@w0/@w1/@w3 et L10_S4_NONCURRENT décodage RawAddress. Les12 headers Core et14 suites historiques restent inchangés.

## Comportements raccordés
- Route d6=3 seulement ; d6=24 conserve son exclusion source @w0. PATCH/MEMORY et autres owners restent exclus.
- État SPOT binaire connu obligatoire. Mode1 SMART ou2 MIX connu obligatoire pour les sources avec champ de mode ; FX conserve le mode1 fixé par @zs. UNKNOWN ne devient pas0.
- DOWN -> LIVE1. SMART : UP avant200ms conserve1, UP à200ms ou après remet0. MIX : le premier UP conserve1 quelle que soit la durée. Si l’état capturé au DOWN était1, UP remet0 dans les deux modes.
- Projection existante OutputS4 réutilisée : notes API neutres MIDI7..11. Pas de notes de synthèse Roland, de binding AUM inventé ou de MIDI Thru.
- Le callback confirme le cache et les dettes. Un callback ON en erreur est conservativement traité comme une note possiblement envoyée et doit recevoir un OFF. Un OFF en erreur conserve sa dette ; prochain STOP ou geste pertinent peut la fermer. Aucun retry générique périodique.
- STOP explicite MIDI0xFC et transition host moving->stopped : le wrapper émet chaque OFF dû, puis confirme individuellement. Les touches restent physiquement Held ; leur vrai UP ne rejoue aucune action. LIVE SMART précédemment0 revient0 au STOP ; les états latched ne sont pas artificiellement assimilés à un état observé.

## Vérification
Tests/test_host_spots.cpp couvre SMART199/200ms, MIX long, MIDI exact, session UNKNOWN, changement de sélection avant UP, ON/OFF échoués et STOP répété, Held au STOP et UP sans fantôme. Tests/test_auv3_document.mm exerce sélection FX -> vrai callback ON -> STOP OFF -> UP inerte dans la vraie classe AU sous macOS.
Le test RED initial échouait sur l’absence d’émission MIDI au DOWN. La nouvelle suite est incluse automatiquement par Tests/run_native.sh.

## Réserves
Ce raccordement ne clôt L04/L06. TOUCH/FLASH (modes3/4), actions/ordonnanceur BAR/BEAT, spécialisations S8 et dettes multi-destinations restent ouverts. Les modes3/4 sont explicitement refusés, sans transformer leur geste en SMART/MIX. La session connue des tests reste une référence logicielle explicite, sans preuve d’authentification de port ou d’état matériel.
Aucun essai Mozaic natif, AUM/iPhone, K2 ou SH-01A nouveau ; recette groupée L15 maintenue. Aucune IPA intermédiaire demandée.

Validation locale : 21 suites C++ GCC16.2 et10 cas C V0.1 PASS ; manifest12/14, K2_SIM SOURCE et profil SH-01A SOURCE PASS. Preuve : evidence/v3/SPOTS_LOCAL_GCC.json. Validation Apple/Clang/sanitizers/iOS à confirmer par la CI du commit.

Cas limite corrigé après test RED : à épuisement du compteur LIVE, un UP qui doit fermer une note déjà envoyée émet tout de même son OFF. La mutation LIVE refusée ne devient pas un faux commit ; la valeur désirée précédente reste conservée. Ce cas de borne n’est pas assimilé à une réconciliation matériel/LIVE.
