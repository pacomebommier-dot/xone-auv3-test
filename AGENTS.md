# XONE AUv3 — consigne de reprise Work / Codex

Avant **toute** modification ou reprise sur cette branche, lire dans cet ordre :

1. `docs/pilotage_v0/00_CONTRAT_PILOTAGE_V0.md`
2. `docs/pilotage_v0/01_CONTRAT_LIVRAISON_RECETTE_V0.md`
3. `docs/pilotage_v0/02_REGISTRE_NON_CONFIGURES_ET_REPRISE.md`

Puis consulter seulement les contrats musicaux GOLDEN / S7 / S8 nécessaires à la tranche en cours et le suivi `docs/reprise/SUIVI_REGROUPE_AVANT_L15.md`.

**Priorité : livrer une V0 AUv3 testable dans AUM**, sans attendre que toutes les destinations, machines, plugins, presets et valeurs de référence soient choisis. Ces inconnues sont volontaires. Utiliser les états `UNKNOWN`, `INVALID` ou `A_CONFIGURER` et les refus sûrs prévus par les contrats ; ne jamais inventer de valeur, de routage ou de succès matériel.

Ne pas recoder GOLDEN/S7/S8 ni modifier les contrats musicaux gelés. Ne pas élargir le périmètre ni lancer d'audit général sans panne précise. Préserver les changements locaux non publiés, vérifier les tests applicables, publier des commits cohérents sur la branche de travail (pas de merge `main`) et renseigner brièvement le registre de reprise.

**Hiérarchie** : les contrats musicaux existants font autorité sur le comportement ; les documents `pilotage_v0` organisent uniquement les priorités, la livraison et les reports. Un build/une CI PASS ne constitue pas une recette AUM/iPhone réalisée.
