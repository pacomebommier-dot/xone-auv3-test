# XONE UNIVERSEL — PLAN DE BATAILLE V1 — 5 BLOCS
Date : 28 octobre 2026. 5 gros blocs de reprise couvrent les 6 tâches historiques ; ne pas afficher artificiellement 6/6 terminées.

## 1/5 — Gestes et sorties MIDI virtuelles exhaustives
Couverture verticale des commandes S1/S2/S3/S4 -> AU : CC/PICKUP, SPOT, TOP/BOT, SOFT, GROUP/LONG, DJ, LOOP, MUTE/SOLO, NEXT_BAR/BEAT et OFF/STOP ; préserver adresses S7 et exceptions S8. Lister seulement les familles manquantes, les raccorder sans refaire S4. Tester les octets par moniteur, sans fixture automatique ni instrument. Sortie : dispatch de toutes les commandes admissibles + CI PASS.

## 2/5 — Banques locales et opérations complètes
PATCH SAMPLE/MACHINE, MEMORY, SAVE, RECALL, slots, CURRENT/ALTERED/EMPTY, CUE/LOAD1..3 -> PATCH, 4 -> BASE local0/OFF/MIX, propriétaires et annulation des MEMORY de la source, PICKUP, LED associés. Respecter TOP immédiat, CUE/LOAD au BAR, MEMORY au BEAT et nuances arrêt transport. Nouveau plugin AU sans banque initiale fictive, mais opération SAVE->RECALL réellement exécutable. Sortie : roundtrip de banque, MIDI nominal, CI PASS.

## 3/5 — PRESET/RED et alternatives S8
Snapshots globaux PRESET1..4/INIT, RED/TEMP, états de mix/FX/MUTE/SOLO, alternatives INST/YELLOW, destinations fixes SAMPLE/DRUMS/FILL, arbitrage, OFF ancienne destination puis activation nouvelle si conditions remplies, sans double autorité ni chargement externe simulé. Respecter exclusion des modes SPOT MACHINE et règle Q10 DJ/track. Sortie : captures/reprises globales + scénarios de concurrence + CI PASS.

## 4/5 — LED, persistance, resynchronisation
Toutes pages LED (notamment MEMORY/PATCH/PRESET/RED), désiré/envoyé/observé sans faux succès. Etat AU natif versionné ; roundtrip save/reopen des banques et domaines autorisés, rejets image corrompue, replay uniquement des états applicables, jamais pulses historiques. Sortie : tests cycle de vie, feedback exact, CI PASS.

## 5/5 — Intégration universelle AUM
K2_SIM ou K2 -> XONE -> moniteur MIDI + feedback et save/reopen ; jeu combiné et STOP, reconnect, timers, simultanéité, temps réel, performances. Paquet iOS signé par utilisateur/outil adéquat, fiche de recette, cas de test. CI source/Apple/iOS PASS ne prouve pas test sur appareil ; obtenir campagne AUM explicite pour déclarer validation. Instruments, presets externes et MIDI Learn restent une phase de configuration ultérieure.

## Règle anti-freeze
Un bloc = un gros parcours livrable, pas des dizaines de mini-lots. Plusieurs commits cohérents possibles sans STOP demandé entre eux. Avant chaque ajout, localiser la fonction existante ; pas de troisième moteur ni d'audit L01-L16. Tests ciblés après changements, CI complète lors de fermeture du bloc et aux risques importants. Si rouge, diagnostiquer première cause, corriger localement, rejouer ; si configuration externe inconnue, classer CONFIG_FUTURE et avancer.
Après chaque bloc : un seul checkpoint (HEAD/SHA, CI, comportements prouvés, réserves, bloc suivant). Aucun GO demandé entre commits. Ne pas prétendre à l'exécution de Work en arrière-plan. Ne pas écraser local non publié, ne pas merge main.

## Démarrage exact
Branche feature/native-e3-bar-beat-reprise. Lire git status, HEAD et modifications locales ; préserver tous les fichiers non publiés. Vérifier sur petit périmètre quelles familles API du S4/AU manquent vraiment, puis commencer directement le code du BLOC1. Ne pas conditionner ce bloc à une configuration Roland ni à l'installation iPhone.
