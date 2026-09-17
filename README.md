# Weather clock

Before building the Keil project, copy `app/config_local.example.h` to
`app/config_local.h` and set your Wi-Fi SSID, Wi-Fi password, and Seniverse API key.
The local configuration file is ignored by Git. Never commit it.

Build `mdk/stm32f103.uvprojx` after creating the local configuration. Compiled
firmware contains these credentials, so `mdk/Objects/` and `mdk/Listings/` must
remain untracked. Do not upload firmware containing your credentials.

To enable checks before committing:

```sh
python -m pip install pre-commit
pre-commit install
```

The secret scan includes custom rules for Seniverse keys in URLs and hardcoded
Wi-Fi passwords. Keep real credentials only in the ignored local header.
