# TÃ¢che 1/6 â€” lot 1A DJ LOOP / GLOBAL

Base : `7ec5887fcebbe222ab55e4b4e51bb429298187ae`, branche `feature/native-e3-bar-beat-reprise`.
CI de base vÃ©rifiÃ©e PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40550179321.

Le resolver S3 existant conserve GLOBAL/LOOP lors de BOT3 + rotation commencÃ©e strictement avant 200 ms. Un PUSH court avec boucle observÃ©e ON demande STOP sans changer de page ; OFF autorise la navigation existante. UNKNOWN conserve la page sans prÃ©tendre avoir observÃ© OFF. Le release d'une rotation qualifiÃ©e ne dÃ©clenche pas de navigation solo.

La dÃ©cision spÃ©cialisÃ©e `DECISION_DJ_LOOP_GLOBAL_P3_V1.txt` impose de conserver boucle et longueur lors de GLOBAL â†” LOOP. Une navigation vers une autre page, y compris l'overlay MUTE natif, demande STOP/RESET. STOP transport invalide toujours les confirmations anciennes. Aucun changement des LOOP1/2/3 permanentes, des gains b1/b2/b3/m ou du routage audio.

La sortie LENGTH reste l'API S7 type16/adresse948 â†’ CC52 relatif. Le vrai dispatcher AU appelle `Result::packet(0)` avant `sent` : cette copie conserve maintenant la qualification DJ2 et l'horodatage DJ1. Sans cette correction, les tests directs de HostEngine ne prouvaient pas le raccordement du callback AU et la fenÃªtre Browser pouvait partir de zÃ©ro.

`pendingDjLoopService` expose une commande START, STOP ou STOP/RESET avec sa gÃ©nÃ©ration ; `acknowledgeDjLoopService` consomme uniquement la remise Ã  un exÃ©cutant. Cette consommation ne confirme jamais un Ã©tat audio. Un nouveau START ou STOP invalide une remise/observation ancienne. L'observation explicite reste distincte. Aucun consommateur audio n'est installÃ© dans le wrapper AU : l'API S7 kind19 est un service sans binding MIDI (`Core/OutputS4.hpp`), pas une commande Loopy vÃ©rifiÃ©e. Ne pas considÃ©rer ce sous-lot comme une boucle audio opÃ©rationnelle dans AUM.

RÃ©gressions : PUSH ON/OFF, GLOBAL/LOOP, combo BOT3â†’BOT2, sortie vers NORMAL/MUTE, UNKNOWN, seuil 200 ms, service consommÃ© une fois, ancienne gÃ©nÃ©ration refusÃ©e, et copie rÃ©elle de paquet. La rÃ©gression originale Ã©chouait sur la sortie GLOBAL ; la rÃ©gression du dispatcher Ã©chouait sur la perte de `dj2Qualified`. Les corrections sont contrÃ´lÃ©es par les suites portables. Compilation iOS et wrapper Apple : CI ; AUM et matÃ©riel : non exÃ©cutÃ©s.

## FrontiÃ¨res Ã  poursuivre dans l'ordre

- 1B : DJ1 Ã©met CC51 ; le deck n'est fourni que par une configuration explicite de test. Le wrapper AU ne dÃ©couvre pas le deck et ne dispose pas d'une API de chargement. Une livraison MIDI ne prouve ni sÃ©lection de fichier ni LOAD rÃ©ussi. FenÃªtre 3 s / owner23 / prioritÃ© sur MUTE conservÃ©s ; l'horodatage AU est corrigÃ© ici.
- 1C : TOP produit des actions capturÃ©es et consommables une fois. `Profiles/SH01A_SOURCE_V1.json` contient des `resetDefaults` et `bindings` vides ; aucun RESET_DEFAULT ne peut Ãªtre inventÃ©. Les banques PATCH natives existent ; leur rÃ©solution et leur exÃ©cution restent Ã  finaliser en tÃ¢che 2. Les intentions TOP ne constituent pas des resets/rappels exÃ©cutÃ©s.

TÃ¢che 1/6 toujours en cours, 0/6 clÃ´turÃ©e. Les dÃ©pendances externes sont isolÃ©es mais ne sont pas prÃ©sentÃ©es comme acceptÃ©es ni rÃ©solues. Les modifications locales prÃ©existantes des inventaires et du test FX sont prÃ©servÃ©es et exclues du commit fonctionnel.
