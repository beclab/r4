docker run --name search3-postgres -e POSTGRES_USER=myuser -e POSTGRES_PASSWORD=mysecretpassword -e POSTGRES_DB=mydatabase -p 5434:5432 -d postgres