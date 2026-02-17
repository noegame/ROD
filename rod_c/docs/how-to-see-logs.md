## Commandes pour voir les logs
### Voir les logs en temps réel
``` shell
# Logs du service de communication seulement
sudo journalctl -u rod-communication.service -f

# Logs des deux services (détection + communication)
sudo journalctl -u rod-detection.service -u rod-communication.service -f
```
### Voir les derniers logs
``` shell
# Les 50 dernières lignes du service de communication
sudo journalctl -u rod-communication.service -n 50

# Les 100 dernières lignes avec horodatage complet
sudo journalctl -u rod-communication.service -n 100 --no-pager
```

### Vérifier le statut des services
``` shell
# Statut du target (vue d'ensemble)
systemctl status rod.target

# Statut détaillé de chaque service
systemctl status rod-detection.service
systemctl status rod-communication.service
```