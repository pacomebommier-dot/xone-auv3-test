# Sélections physiques et LOOP — V3, 26/10/2026

Base : d30bb1d ; CI complète PASS, session LIVE via document AUv3 implémentée et vérifiée en harness. AUM réel toujours différé.
Le test préalable fc7fcd65 est volontairement RED : le host n'avait ni sélection @zk/@zl activée ni résultat MIDI multiple pour LOOP.

## Raccordements
Le host active la sélection NAV @zk/@zl déjà portée dans Core/S3TouchNav.hpp. Aucun Core historique réécrit. Un appui K2 de sélection modifie les routes des CC suivants ; les priorités BOT, la fenêtre 200ms, les vrais UP et STOP restent ceux de la source. Ne pas confondre TOUCH NAV (sélection de source) et SPOT TOUCH/FLASH musical, qui restent à intégrer.

LOOP : cellules LIVE1337/1338/1339 = b1/b2/b3, 1341 = m. API569/570/571 = (bi*m+63)/127 arrondi entier, calculé sur une même révision LIVE préparée. Un changement bi réévalue son gain ; un changement m réévalue les trois gains. API573 n'est jamais émise comme gain autonome, API572 reste réservée.
Les composantes UNKNOWN restent inconnues ; seules les paires connues peuvent produire une sortie. Le LIVE désiré ne constitue pas une observation matérielle.

Un résultat host contient au maximum trois paquets statiques. Le callback AUv3 les envoie dans l'ordre et confirme le cache de chaque paquet seulement après son propre succès. Un échec d'un gain n'empêche pas de confirmer les autres ; le prochain mouvement identique réémet seulement celui qui manque. Pas de retry automatique, ni d'allocation ou file extensible.

## Oracles et tests
Source S7 S3 @zk/@zl pour sélection, @zp/@zc/@x1 pour LOOP, API MIDI virtuelle V1.2 section LOOP.
Tests/test_host_musical_routes.cpp :
- CC4 bus API907 -> appui physique note36 -> CC4 source API715 ; STOP ne fabrique pas le relâchement qui désélectionnerait la source ;
- combo BOT3 puis BOT2 -> mode LOOP4 ;
- facteur127 puis64 : gains127/64/32 puis64/32/16, sur MIDI11 CC57/58/59 ;
- échec du deuxième callback : seul API570 est réémis au mouvement suivant ;
- changement b1 : seul gain569 réévalué ; aucun CC573 et aucun multiplicateur0 inventé.

La CI du commit doit être vérifiée séparément. Pas de recette AUM, LED, synthé ou validation K2 physique déduite de ces tests. PATCH/MEMORY/CUE/LOAD/PRESET/RED/S8, sorties SPOT/actions, horloge BAR/BEAT et profils matériels restent ouverts. Le plan 8 étapes/16 lots est conservé, sans clôture abusive.

CI complète3e901da : PASS (tentative2, après timeout d'archivage) https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40397913253 . Le code musical n'a pas été modifié pour résoudre l'erreur réseau.
