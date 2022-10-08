use chrono::NaiveDateTime;
use sqlx::{
    self,
    postgres::{PgPoolOptions, Postgres},
};

#[derive(Debug, sqlx::FromRow)]
pub struct Course {
    pub id: i32,
    pub teacher_id: i32,
    pub name: String,
    pub time: Option<NaiveDateTime>,
}

async fn insert_with_transaction(
    transaction: &mut sqlx::Transaction<'_, sqlx::Postgres>,
    teacher_id: i32,
    name: &str,
) -> Result<(), Box<dyn std::error::Error>> {
    let insert_sql = sqlx::query(r#"INSERT INTO COURSE(teacher_id,name) values ($1,$2)"#)
        .bind(teacher_id)
        .bind(name);

    insert_sql.execute(&mut *transaction).await?;

    // let query_sql =
    //     sqlx::query(r#"SELECT id,teacher_id,name,time  FROM COURSE WHERE name = $1"#).bind(name);

    // println!("{:?}", query_sql.sql());
    // let res = query_sql.fetch_all(&mut *transaction).await?;

    // for row in res {
    //     for column in row.columns() {
    //         println!(
    //             " {:?},{:?},{:?}",
    //             column.name(),
    //             column.type_info(),
    //             // value? //  row.try_get_raw("id")?.type_info()
    //             row.try_get::<i32, _>(column.name())?
    //         );
    //     }
    // }

    // let list2 = sqlx::query!(r#"select * from course where name = $1"#, name)
    //     .fetch_all(&mut *transaction)
    //     .await?;
    // let mut vec2 = vec![];
    // for row in list2 {
    //     vec2.push(Course {
    //         id: row.id,
    //         teacher_id: row.teacher_id,
    //         name: row.name,
    //         time: row.time,
    //     })
    // }

    // println!("{:?}", vec2);

    let res = sqlx::query_as::<Postgres, Course>(
        r#"SELECT id,teacher_id,name,time  FROM COURSE WHERE name = $1"#,
    )
    .bind(name)
    .fetch_all(transaction)
    .await?;

    println!("{:?}", res);

    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let pool = PgPoolOptions::new()
        .max_connections(5)
        .connect("postgres://postgres:postgres@127.0.0.1:5432/postgres")
        .await?;

    let mut tr = pool.begin().await?;
    insert_with_transaction(&mut tr, 5, "gkonds").await?;
    tr.commit().await?;
    Ok(())
}
