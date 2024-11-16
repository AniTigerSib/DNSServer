# Настройка всей конфигурации

## 1 Скачивание готового ELK-контейнера

`docker pull sebp/elk`

## 1 Установка Filebeat

`docker pull docker.elastic.co/beats/filebeat:8.10.2`


## 2 Запуск контейнера



## 2 Создание конфигурационного файла Filebeat

Заменяем в filebeat.yml `- /path/to/dns-logs/*.log` на путь к логам

## 3 Запуск filebeat 

Запускаем докер образ командой
```
docker run -d \
    --name=filebeat \
    --user=root \
    --volume=/path/to/filebeat.yml:/usr/share/filebeat/filebeat.yml \
    --volume=/path/to/dns-logs:/usr/share/filebeat/logs \
    --network=host \
    docker.elastic.co/beats/filebeat:8.10.2
```
