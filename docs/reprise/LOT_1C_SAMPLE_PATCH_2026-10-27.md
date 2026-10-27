# Tâche 1/6 — lot 1C : exécutant PATCH SAMPLE

Base : `d648aa8`, branche `feature/native-e3-bar-beat-reprise`. Les modules historiques restent inchangés.

TOP1–TOP3 dans le contexte SAMPLE utilisent maintenant les banques PATCH SAMPLE existantes (`BankS6`, owners 0..7, slots 0..2). Une banque valide et une configuration de session correspondante sont requises. L'intention acceptée est consommée une fois et son owner/slot/révision sont capturés. Une seule demande peut être en cours ; les autres intentions TOP restent disponibles sans être exécutées ni déclarées réussies.

Au prochain vrai BAR fourni par HostGrid, S3 prépare les 40 champs dans une copie du LIVE et ne publie que si toute l'image est acceptable. S4 compose les sorties existantes : 16 CC (API16*owner+champ) et 12 états SPOT (API12*owner+position), jusqu'à 28 paquets MIDI. Les 12 modes SAMPLE viennent du PATCH sans créer de geste physique. La déduplication S4 et les confirmations MIDI individuelles restent actives. L'application logicielle désirée ne vaut pas observation audio.

Le wrapper AU appelle cet exécutant dans `deliverGridThrough`, avant le MIDI portant exactement le même timestamp : une demande ne peut pas s'armer rétroactivement sur ce BAR. Aucun BAR n'est fabriqué lors d'une perte d'horloge. STOP et restauration de session annulent la demande sans effacer la banque.

Un SPOT cible tenu, une transaction NAV/TOUCH en cours, un profil ou une révision incompatibles font refuser avant mutation et avant émission. Le refus ne supprime pas les OFF physiques dus. Les callbacks ON échoués conservent leur dette OFF selon le mécanisme existant.

## Portée restante

`storeSamplePatchBeforeRendering` raccorde explicitement l'entrée des banques connues au moteur avant rendu. Il n'importe aucun ancien LIVE et n'émet rien. Les gestes de sauvegarde, l'entrée des banques depuis le wrapper et leur persistance restent dans les tâches 2/5 ; une AU neuve ne possède donc pas de PATCH SAMPLE valide par défaut. Ce parcours n'est pas encore une chaîne SAVE → RECALL autonome dans AUM.

La référence est le payload SAMPLE40 de la matrice PATCH/BASE du 15 octobre. La décision D-S8-MODES-MACHINE-EXCLUSIFS du 19 octobre ne s'étend pas à SAMPLE. MACHINE reste exclu de ce raccordement : le payload legacy y contient des modes qui ne doivent plus être capturés/restaurés. Son adaptateur natif devra respecter la configuration S8 actuelle, les champs BASE hors payload et le désengagement MEMORY.

Application ici au BAR uniquement ; le comportement au repos transport et les interactions complètes CUE/LOAD/BASE restent à finaliser avec les commandes de banques en tâche 2. Les defaults individuels FILTER restent UNKNOWN et aucun RESET zéro n'est déduit de ce rappel.

Régressions : octets MIDI et sample time, 40 champs LIVE et exclusions, banque invalide/profil/révision, BAR dupliqué, seek arrière HostGrid avec ancrage sans commande puis premier vrai BAR, BEAT sans application, consommation unique, cible capturée après sortie de page, callback ON échoué/STOP, TOUCH tenu/OFF dû, échec tardif sans LIVE partiel, restauration de session. SOURCE et iOS ne valent pas recette AUM/matériel.

Tâche 1/6 toujours en cours, 0/6 clôturée. Aucun succès matériel ni dépendance externe acceptée par déduction.

Validation locale avant publication : 51 suites C++, les 10 tests C et les contrôles SOURCE/profils/simulateur PASS ; la régression seek a reproduit le blocage avant sa correction. Les deux suites SAMPLE sont revérifiées après la correction. CI Apple/iOS à vérifier sur le commit publié.
