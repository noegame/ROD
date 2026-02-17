# How to Edit WiFi Networks with Netplan


Voici comment ajouter un réseau WiFi avec netplan en runtime (sans redémarrage) :
Procédure complète

1. Éditez le fichier de configuration netplan:​
```bash
sudo nano /etc/netplan/50-cloud-init.yaml
```
2. Ajoutez votre nouveau réseau WiFi dans la section access-points:​
```bash
text
network:
  version: 2
  wifis:
    wlan0:
      access-points:
        "ANCIEN_WIFI":
          password: "ancien_pass"
        "NOUVEAU_WIFI":
          password: "nouveau_pass"
      dhcp4: true
```
3. Appliquez la configuration immédiatement:
```bash
bash
sudo netplan apply
```
Cette commande génère les fichiers de configuration backend (wpa_supplicant) et active le nouveau réseau immédiatement sans reboot. Le processus invoque automatiquement les démons appropriés (systemd-networkd ou NetworkManager) pour activer les interfaces configurées.​
Syntaxe pour plusieurs réseaux

Vous pouvez lister autant de réseaux que vous voulez :​
```bash
text
access-points:
  "Maison":
    password: "pass1"
  "Bureau":
    password: "pass2"
  "Portable_4G":
    password: "pass3"
```
Le système se connectera automatiquement au premier réseau disponible dans la liste.