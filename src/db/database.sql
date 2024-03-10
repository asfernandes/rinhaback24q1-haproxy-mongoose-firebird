create table transaction (
    account_id integer not null,
    seq integer not null,
    balance integer not null,
    overdraft integer not null,
    val integer not null,
    description varchar(10) not null,
    datetime timestamp with time zone not null,
    constraint transaction_pk primary key (account_id, seq) using desc index transaction_pk_desc
);


create or alter view transaction_check
as
select *
    from (
        select t.*,
               sum(val) over (partition by account_id order by seq) calc_balance
            from transaction t
    )
    where balance < overdraft or
          calc_balance <> balance
    order by account_id, seq;


set term !;

create or alter procedure post_transaction(
    account_id integer,
    val integer,
    description varchar(10)
) returns (
    status_code integer,
    balance integer,
    overdraft integer
)
as
    declare account_exists boolean = false;
begin
    while (true) do
    begin
        in autonomous transaction do
            insert into transaction (account_id, seq, balance, overdraft, val, description, datetime)
                select :account_id,
                        seq + 1,
                        new_balance,
                        overdraft,
                        :val,
                        :description,
                        'now'  -- current_timestamp
                    from (
                        select seq,
                               balance + :val new_balance,
                               overdraft
                            from transaction
                            where account_id = :account_id
                            order by seq desc
                            rows 1
                    )
                    where new_balance >= overdraft
                returning balance, overdraft
                into balance, overdraft;

        if (balance is not null) then
            status_code = 200;
        else if (account_exists) then
            status_code = 422;
        else
        begin
            status_code = 404;

            select 422
                from transaction
                where account_id = :account_id and
                      seq = 0
                into status_code;
        end

        exit;
    when gdscode unique_key_violation do
        account_exists = true;
    end
end!

set term ;!


insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (1, 0, 0, -100000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (2, 0, 0, -80000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (3, 0, 0, -1000000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (4, 0, 0, -10000000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (5, 0, 0, -500000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
