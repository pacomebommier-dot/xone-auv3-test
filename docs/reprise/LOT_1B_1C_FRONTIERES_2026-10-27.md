# Tâche 1/6 — LOAD et sorties TOP/BOT

Commit fonctionnel : `50fea91e0e4ff7f6b2d2cac368dc5d7b6f04f743`.
CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40578887558.
GCC, Clang, Apple Clang, ASan/UBSan, vrai wrapper AU macOS, build iOS non signé et artefacts : PASS. Les 50 suites C++ du glob CI et les tests C sont exercés. Cinq suites E3 ciblées finales PASS localement avec GCC, assertions et warnings fatals. Ce checkpoint ne valide pas les fonctions externes ci-dessous.

## Lot 1B

`HostEngine::BrowserLoadRequest` ajoute une remise de commande à usage unique : deck explicitement configuré, séquence du dernier CC51 livré et séquence du PUSH propriétaire. `pendingBrowserLoad` permet la lecture ; `consumeBrowserLoad` refuse anciennes demandes et doubles consommations. Nouvelle sélection et STOP annulent la demande non remise. Fenêtre de 3 s, owner23, vrai DOWN/UP et priorité sur MUTE sont conservés.

Il n'existe pas d'API de fichier/chargement ni de découverte du deck sélectionné dans `Extension/XoneMidiAudioUnit.mm`. S7 kind19 LOAD est stocké comme service par `OutputS4`, sans binding MIDI ni commande de chargement externe. La remise native est une frontière pour un exécutant futur, pas un chargement réussi ni une sélection de fichier confirmée. Aucun état loaded/success n'est fabriqué.

Régression par le chemin réel `Result::packet(0)` : sélection à 5000 ms, DOWN avant expiration, UP après expiration, deck 2, exclusion MUTE, consommation unique, ancienne séquence refusée après nouvelle rotation et STOP.

## Lot 1C

Tests par les vrais gestes S1/S2/S3 : TOP1 NORMAL vise INST1, TOP1 YELLOW vise YELLOW1 ; TOP1 SAMPLE et TOP1–3 MACHINE restent PATCH1–3 ; PRESET/RESET conservent le contexte SAMPLE inférieur ; TOP4 conserve la même route GLOBAL dans ces contextes. UNKNOWN reste UNKNOWN, sans paquet musical de reset fabriqué.

Autorité : `XONE_K2_REBUILD_ISO/04_NEW_CHAIN/01_CONTRATS/TABLE_RESET_DEFAULT_NEW_V1.txt`, sections 2, 4 et 9. Seuls les EQ NORMAL 4..9 et YELLOW 127..132 sont prouvés à 63. Les defaults FILTER restent inconnus. `CONTRAT_RESET_DEFAULTS_NEW_V1.5.txt` §1 exige le refus avant mutation si default inconnu. Un RESET FILTER exécuté attend donc une référence prouvée ; le binding externe n'est pas déduit d'un numéro MIDI.

Les banques PATCH de `Core/BankS6.hpp` démarrent INVALID/UNKNOWN. Le raccordement SAMPLE au BAR vers LIVE/S4 est désormais décrit dans `LOT_1C_SAMPLE_PATCH_2026-10-27.md`, sous réserve d’une banque explicitement fournie avant rendu. Les commandes SAVE et la fourniture des banques dans le wrapper restent à finaliser en tâche 2 ; MACHINE attend son adaptateur excluant les modes S8. Une intention seule ne vaut pas un rappel exécuté. Les BOT conservent NAV/combos/MUTE ; PUSH DJ2 et sortie vers MUTE sont couverts par 1A. Les propriétaires PRESET/RED spécialisés restent en tâche 3.

Les frontières DJ LOOP audio, sélection/deck/LOAD et defaults FILTER ne sont pas déclarées acceptées ni opérationnelles. Tâche 1/6 en cours, 0/6 clôturée. Aucun PASS AUM ou matériel.
