BEGIN;

CREATE TABLE users(
  id serial primary key not null,
  email varchar not null,
  crypted_password varchar not null,
  password_salt varchar not null,
  name varchar not null
);

CREATE TABLE posts(
  id serial primary key not null,
  created_at timestamp with time zone not null,
  updated_at timestamp with time zone,
  published_at timestamp with time zone not null,
  author_id integer references users(id) not null,
  title varchar not null,
  "text" text not null
);

CREATE TABLE comments(
  id serial primary key not null,
  created_at timestamp with time zone not null,
  updated_at timestamp with time zone,
  "text" text not null,
  author_id integer references users(id) not null,
  post_id integer references posts(id) not null
);

CREATE UNIQUE INDEX users_email_index ON users(email);

COMMIT;
