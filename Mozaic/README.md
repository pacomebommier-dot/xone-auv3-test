# K2_SIM V3 — préparation SOURCE

Une seule instance Mozaic, route exclusive Mozaic -> XONE. Ne pas activer simultanément le K2 physique. Le retour API/LED du moteur ne doit pas être routé vers son entrée. La sortie L02 CH1 est seulement un banc historique.

Charger K2_SIM_V3.txt dans l'éditeur Mozaic à la recette groupée L15. Aucun essai iPhone demandé maintenant. SOURCE contrôlée ; compilation Mozaic/AUM non réalisée.

- Pads0..3 : BOT1..4, toujours accessibles.
- Knob7 : page boutons 0..2. Pads4..15 : 12 boutons de cette page (dernière page : 8 actifs).
- Knobs0..5 : CC0..5 au départ ; SHIFT GUI passe aux CC6..11, 12..17 puis 18..21. Les contrôles excédentaires sont inactifs. CC20/21 = DJ1/2 bruts.
- Knob6 : inutilisé. Les étiquettes indiquent le numéro K2, pas une commande musicale.
- Maintenir plusieurs pads pour les combos. Les durées sont celles des gestes réels ; S2 les qualifie dans le moteur.

Changer de page pendant une tenue conserve la note à relâcher. STOP ne simule pas de UP ; relâcher réellement les pads. Pas de reset/panic musical dans Mozaic, aucune sortie instrument/LED ni calcul PATCH/LIVE.
