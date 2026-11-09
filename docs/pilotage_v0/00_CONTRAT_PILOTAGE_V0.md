# XONE AUv3 — CONTRAT DE PILOTAGE V0
Version 1.0 — 28 octobre 2026 — document normatif de pilotage, à relire à chaque reprise

## 1. Objet
Livrer **au plus vite une XONE AUv3 V0 installable et utilisable dans AUM**, puis faire évoluer le plugin. La livraison V0 prime sur la clôture exhaustive des six regroupements. Les six tâches restent la feuille de route du produit complet, non une condition préalable à V0.

Ce document **ne remplace ni ne modifie** les contrats musicaux GOLDEN / S7 / S8, les affectations et décisions déjà gelées. En cas de contradiction musicale, les contrats métier existants font autorité ; signaler la contradiction au lieu de réinterpréter silencieusement.

## 2. Obligations non négociables
1. **Réutiliser, ne pas recoder.** Avant tout ajout, identifier la fonction déjà présente dans GOLDEN, S7/S8 et le moteur natif, puis ne développer que le raccordement réellement manquant vers l'AUv3. Pas de troisième moteur, de double ordonnanceur ou de nouvelle architecture de substitution sans nécessité démontrée.
2. **Inconnues intentionnelles.** Plugins, instruments, sources audio, destinations MIDI, presets, BASE/RESET, PATCH et valeurs de profil non encore choisis sont des paramètres à configurer plus tard, **pas des bugs à combler maintenant**.
3. **Pas d'invention.** Ne jamais remplacer UNKNOWN / INVALID / NOT_CONFIGURED par zéro, un preset Roland imaginaire, un port MIDI arbitraire, un ACK fabriqué ou un état « audio playing / loaded » non observé.
4. **Dégradation locale.** Une action dont la ressource manque refuse proprement **cette action seulement** et ne bloque pas les autres parcours. Préserver les OFF dus, les fermetures STOP, l'ownership et les invariants de sécurité même lors d'un refus ou d'un échec MIDI.
5. **Pas de faux routage audio.** XONE pilote les commandes, les états et les sorties MIDI. Il ne transporte pas ni ne mixe lui-même les flux audio des instruments AUM.
6. **Non-régression.** Ne pas toucher aux archives, empreintes, headers et contrats gelés. Conserver les suites existantes, les contrôles d'intégrité, les sanitizers et le vrai wrapper AU. Aucune désactivation de tests pour produire un PASS.
7. **Priorité au résultat installable.** Les fonctions secondaires et profils non configurés ne doivent pas prolonger indéfiniment la préparation de V0. Les interfaces sûres restent prêtes pour V0.1 et suivantes.

## 3. Classement obligatoire d'un écart
Chaque écart repéré est classé **une seule fois** :
- **BLOQUANT V0** : empêche de construire, ouvrir, parcourir ou arrêter sûrement l'AUv3 dans AUM, ou casse un test indispensable.
- **RACCORDABLE V0** : fonction existante avec liaison AU/MIDI essentielle et limitée, à raccorder sans refonte.
- **A_CONFIGURER** : dépend d'une machine, d'un preset, d'une valeur ou d'une destination absente ; garder UNKNOWN/refus sûr, inscrire au registre.
- **DIFFERE** : fonctionnalité non indispensable au parcours V0, à conserver pour une version ultérieure.

Une dépendance externe non disponible n'est **pas** automatiquement BLOQUANT V0. Le propriétaire de la tâche doit justifier pourquoi elle empêcherait l'ouverture ou l'utilisation des parcours disponibles.

## 4. Frontières à ne pas franchir
- Ne pas déplacer implicitement les fonctions de la tâche 2 (PATCH / MEMORY / CUE / LOAD / BASE) dans la tâche 1. Si une portion PATCH est déjà commencée, la sécuriser et la ranger clairement ; ne pas élargir le périmètre.
- DJ LOOP externe et Browser LOAD : une **demande** de service n'est ni une exécution effective ni une lecture/charge observée. Sans exécuteur réel, interface sûre, état explicite « non raccordé », et poursuite de la V0.
- FILTER / BASE / RESET inconnus : refus sans mutation et sans MIDI deviné. La calibration pourra intervenir après V0.
- Les modes MACHINE exclus des snapshots par décision S8 restent exclus. Ne pas généraliser à SAMPLE si son contrat prévoit autre chose.
- Pas de fusion vers `main` sans demande explicite. Conserver les modifications locales non publiées lors de toute reprise.

## 5. Décision de travail
À chaque nouvelle demande de code, répondre d'abord en une phrase à : **« Est-ce indispensable à la V0, ou est-ce seulement l'achèvement du produit final ? »**
Si ce n'est pas indispensable, classer et poursuivre le chemin critique. Le nombre de commits, de tests nouveaux ou de documents produits n'est pas un indicateur d'avancement V0.
