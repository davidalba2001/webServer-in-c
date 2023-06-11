# Como ejecutar el proyecto

## Crear el binario :

 ```bash
    make
 ```

## Existen 3 formas de ejcutarlo
> Inicia por defecto en el port:8080 y en el path /home
```bash
    make run
 ```
>  Esta forma permite selecionar el puerto y el path de inicio 
```bash
    make run ARGS="<port> <path>"
 ```
> forma normal sin usar make
```bash
    ./bin/webServer.out <port> <path>
 ```
