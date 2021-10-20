**Note: Development has moved to the 'develop' branch to be consistent with the other OSP repositories.**

Please help complete these instructions.

### Building

```
composer update --with-all-dependencies
composer install
php artisan config:cache
php artisan key:generate
php artisan config:cache
php artisan migrate --force
```

### Running

```
./artisan serve --host=0.0.0.0 --port=8080
```
