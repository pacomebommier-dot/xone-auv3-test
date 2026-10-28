# XONE AUv3 — CONSIGNE PERMANENTE DE REPRISE WORK / CODEX

## Lire impérativement avant toute modification (ordre de priorité)
1. docs/universel/00_CONTRAT_MAITRE_UNIVERSEL_V1.md — règle produit universel, priorité active depuis le 28/10/2026.
2. docs/universel/01_CONTRAT_API_ET_ADAPTATEURS_V1.md — MIDI virtuel fixe, adaptation AUM et profils matériels séparés.
3. docs/universel/02_PLAN_BATAILLE_5_BLOCS_V1.md — 5 blocs de chantier, sans mini-lots.
4. docs/universel/03_KICKOFF_WORK_EXECUTABLE_V1.md — geste de développement et cadence.

Puis, **seulement pour leurs gardes techniques et leurs précisions compatibles** :
- docs/pilotage_v0/00_CONTRAT_PILOTAGE_V0.md
- docs/pilotage_v0/01_CONTRAT_LIVRAISON_RECETTE_V0.md
- docs/pilotage_v0/02_REGISTRE_NON_CONFIGURES_ET_REPRISE.md
- docs/reprise/SUIVI_REGROUPE_AVANT_L15.md (traçabilité historique des 6 tâches, pas le nouveau compteur).
Les contrats musicaux GOLDEN, NEW, S7, S8 décident toujours du comportement sur leur périmètre. Lire uniquement ceux utiles au bloc courant, pas de revue intégrale à chaque reprise.

## Règle fondamentale
**XONE commande, mémorise, restitue et émet. AUM distribue. Les instruments interprètent.**
Toutes les pages, slots, banques, gestuelles, CC/Notes, PATCH, MEMORY, CUE/LOAD, BASE, PRESET, RED, LED, PICKUP et persistance doivent fonctionner et être testables en MIDI virtuel sans instruments/plugins/presets choisis.
Les sorties neutres normatives (API S7 V1.2 MIDI7..14, MIDI15 LED) restent disponibles sans binding musical. GOLDEN/CCOUT est la provenance, non la numérotation finale lorsque supplantée. AUM MIDI Learn et les adaptateurs de machines sont la couche de configuration ultérieure ; XONE n'effectue aucun routage audio.

## Inconnues et sécurité
Ne jamais transformer UNKNOWN/INVALID en 0, synthé hypothétique, CC/PC/SysEx imaginaire ou succès de service matériel. BASE local provisoire0/OFF/MIX explicitement décidé est distinct du RESET_DEFAULT individuel inconnu. Une absence d'instrument peut rendre la **réussite externe** non vérifiable, mais ne retire pas la commande interne ni son émission API neutre si celle-ci est admissible.
Les profils spécialisés S8 et leurs exclusions sont conservés. Maintenir NoteOff dus, owners, STOP, transitions, horloge, LED et protections de persistance ; refuser localement une action qui exigerait une vraie référence matérielle inconnue.

## Workflow anti-freeze
Commencer bloc 1/5, ne pas refaire audit L01–L16 ; vérifier HEAD et protéger le non-publié. Réutiliser le moteur existant ; pas de second scheduler. Commits cohérents, tests ciblés, CI complète à la clôture du bloc/risque important ; continuer automatiquement entre commits sans demander GO. Corriger localement première erreur CI avant de poursuivre. Checkpoint bref avec SHA/CI et prochain geste concret, pas de multiplication de docs.
Ne jamais désactiver des tests ni modifier archives/empreintes gelées. Pas de merge main sans accord explicite. CI PASS ≠ recette AUM sur appareil.

## Hiérarchie de pilotage
Ces documents universel sont les ADDENDA actifs de pilotage ; ils remplacent uniquement la vieille classification « binding absent => fonction interne différée » de pilotage_v0. Les décisions musicales GOLDEN/S7/S8 restent normatives. En conflit métier réel, ne pas trancher par invention : exposer l'écart précis et préserver la sécurité.
