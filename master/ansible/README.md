# Bootstraping

- determine ip address of raspberry, e.g. inspect dhcp logfile
- enter ip in hosts file
- run bootstrap playbook

        :::sh
        ansible-playbook bootstrap.yml -i hosts --ask-pass
        
    
    --vault-password-file ~/.vault_pass.txt

## Config bootstraping playbook

Adapt the secrets in group_vars/newhosts/secrets.yml

        :::yml
        ---
        raspi_wifi_id:   'a symbolic name'
        raspi_wifi_ssid: 'your ssid'
        # can be a plain password or better a encrypted psk, eg. calculated with
        # https://www.wireshark.org/tools/wpa-psk.html
        raspi_wifi_psk:  'passwd or encrypted psk'


# Run specific playbooks
