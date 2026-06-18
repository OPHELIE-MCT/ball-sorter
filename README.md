# Ball Sorter

Ce dépôt contient le firmware du sous-système de tri des balles du projet M2614. Il est prévu pour une carte de type Seeeduino Nano et pilote le mécanisme local de tri de manière autonome.

Le trieur ne remplace pas le firmware principal du robot. Il forme un sous-ensemble spécialisé qui reçoit les balles, les observe, les classe par couleur et actionne la mécanique de déviation.

## Rôle du trieur

Le trieur gère localement :

- le moteur du mécanisme d'avance des balles
- le servo qui dirige la balle vers la bonne sortie
- deux capteurs ToF de présence
- le capteur spectral AS7341 pour la classification couleur
- l'anneau NeoPixel de retour visuel

Le firmware principal `M2614_LaFaceCacheeDeLaLune` se contente d'activer ou non le trieur. Toute la logique de détection locale, de classification et de mouvement interne est réalisée ici.

## Matériel cible

Le code est conçu pour la plateforme et le banc du projet M2614 :

- carte type Seeeduino Nano
- capteur couleur AS7341
- deux capteurs de distance VL6180X
- un servo de déviation
- un moteur DC d'entraînement
- un anneau NeoPixel

Les principales broches utilisées sont déclarées au début de `ball-sorter.ino`.

## Principe de fonctionnement

Le trieur suit une séquence simple et robuste :

1. un capteur ToF détecte l'arrivée d'une balle
2. le moteur avance la balle jusqu'à la zone d'observation
3. l'AS7341 lit les canaux spectraux
4. le classifieur embarqué compare les mesures aux centroïdes calibrés
5. le servo oriente la balle selon le résultat
6. les NeoPixels affichent la couleur détectée et un niveau de confiance visuel

Dans l'état actuel du projet, le tri est pensé pour isoler les balles rouges du reste du lot :

- angle servo nominal : `55` degrés
- angle servo rouge : `105` degrés

## Ce que signifient les retours visuels

### Affichage normal

Les NeoPixels affichent la couleur prédite. Plus la confiance est haute, plus un grand nombre de pixels sont allumés.

### Arrêt d'urgence

Si l'entrée d'arrêt d'urgence est activée, le moteur est coupé et le trieur affiche une animation d'erreur rouge.

### Erreur capteur

Si un ToF ou le capteur AS7341 n'est pas détecté correctement au démarrage, le trieur passe en état d'erreur capteur et affiche une animation magenta.

## Compilation et téléversement

### Compilation

Commande minimale :

```powershell
arduino-cli compile -b Seeeduino:avr:nano --build-path $env:TEMP\arduino-build
```

### Téléversement

Commande correspondant à la configuration actuelle de l'atelier :

```powershell
arduino-cli upload --verbose -p COM28 -b Seeeduino:avr:nano --input-dir $env:TEMP\arduino-build
```

`COM28` est une valeur propre à la machine de développement actuelle. Elle doit être adaptée si le port série change.

### Debug série

Le sketch ouvre `Serial` à `115200` bauds. C'est le point d'entrée principal pour comprendre :

- si les capteurs sont détectés au démarrage
- si le capteur AS7341 renvoie bien des mesures
- quand une première balle est détectée
- quand le mécanisme réinitialise son état interne après temporisation

## Comportement détaillé

### Détection et avance des balles

Le moteur est activé quand :

- une commande manuelle de rotation est présente
- ou bien quand le ToF amont voit une balle qui n'est pas encore considérée comme présente au niveau du capteur couleur

Au démarrage du moteur, le sketch applique une courte impulsion pleine puissance de `10 ms` pour faciliter le lancement mécanique, puis repasse à une tension cible équivalente à environ `3 V`.

### Classification couleur

Le classifieur embarqué travaille sur un vecteur de `10` mesures dérivées du capteur AS7341. Ces mesures sont comparées à `7` classes connues :

- orange
- purple
- blue
- green
- yellow
- pink
- red

Le détail de l'API de classification est documenté dans `classification.h`.
Le bloc de paramètres générés (classes, centroïdes, rayons de confiance) est isolé dans `config.h`.

Deux seuils importants coexistent :

- `BallClassifier::kInnerConfidenceRadius` et `kOuterConfidenceRadii` dans `config.h` pilotent la confiance: 100% dans le rayon interne global, puis décroissance linéaire jusqu'au rayon externe de la classe la plus proche, au-delà duquel l'échantillon devient `unknown`
- `kUnknownBallThreshold = 0.35` dans `ball-sorter.ino` sert ensuite de seuil pratique pour décider si la confiance est suffisante pour considérer qu'une balle est bien présente au niveau du capteur couleur

## Recalibrage

Les centroïdes du classifieur embarqué ne sont pas ajustés automatiquement sur la carte. Pour les mettre à jour, il faut passer par les deux autres dépôts du projet :

1. `M2614_LaFaceCacheeDeLaLune` capture les mesures AS7341 côté Uno Q
2. `M2614_LaFaceCacheeDeLaLune-Python` étiquette les échantillons et lance l'analyse des centroïdes
3. le bloc C++ généré est recopié dans `config.h` (fichier dédié, pensé pour être vidé puis recollé tel quel)
4. ce dépôt est recompilé et retéléversé sur la Seeeduino Nano

Le dépôt `ball-analyzer` reste une référence utile pour l'analyse hors ligne et les vérifications supplémentaires, mais ce n'est plus le workflow nominal de l'utilisateur final.

## Limitations connues

- la qualité du tri dépend directement de la qualité du recalibrage
- les centroïdes embarqués sont statiques tant que `config.h` n'est pas mis à jour
- le trieur ne publie pas ici de diagnostic haut niveau vers une interface utilisateur distante
- si le matériel change, les temporisations mécaniques peuvent devoir être reprises

## Dépannage rapide

### Rien ne bouge

- vérifier l'alimentation du moteur et du servo
- vérifier l'état de l'entrée d'arrêt d'urgence
- vérifier le port série à `115200` pour confirmer le bon démarrage du sketch

### Animation magenta en boucle

- vérifier la présence des deux capteurs VL6180X
- vérifier la présence du capteur AS7341
- vérifier le câblage I2C et l'initialisation des adresses capteur

### Les balles sont mal triées

- vérifier d'abord que les couleurs détectées semblent cohérentes dans les NeoPixels
- si les prédictions sont instables, refaire une campagne de recalibrage
- vérifier que `config.h` contient bien les derniers centroïdes valides

### Le téléversement échoue

- vérifier le port série réel à la place de `COM28`
- vérifier que la bonne carte est `Seeeduino:avr:nano`

## Fichiers importants

- `ball-sorter.ino` : sketch principal du trieur
- `classification.h` : API publique du classifieur couleur
- `config.h` : constantes générées (classes, centroïdes, rayons de confiance) à remplacer lors d'un recalibrage
- `classification.cpp` : logique de classification embarquée qui consomme ces constantes

## Documentation Doxygen

La documentation Doxygen de ce dépôt combine :

- ce README comme page d'accueil
- l'API documentée du classifieur
- le sketch principal du trieur pour la référence de maintenance
