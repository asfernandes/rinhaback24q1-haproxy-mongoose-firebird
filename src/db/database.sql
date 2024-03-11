create table transaction (
    account_id integer not null,
    seq integer not null,
    balance integer /*not null*/,
    overdraft integer /*not null*/,
    val integer /*not null*/,
    description varchar(10) /*not null*/,
    datetime timestamp with time zone /*not null*/,
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
begin
    while (true) do
    begin
        /*
        If this does not raise exception, it means one of these:
        - the transaction succeded (status_code = 200)
        - bankrupt (only when val < 0)
        - account does not exists
        */
        in autonomous transaction do
            insert into transaction (account_id, seq, balance, overdraft, val, description, datetime)
                select :account_id,
                        seq + 1,
                        new_balance,
                        overdraft,
                        :val,
                        :description,
                        'now'   -- with current_timestamp it will be fixed during the whole execution
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
                returning 200, balance, overdraft
                into status_code, balance, overdraft;

        if (status_code is null) then
            status_code =
                case
                    when val < 0 then
                        coalesce(
                            (select 422  -- account exists, so it is bankrupt
                                 from transaction
                                 where account_id = :account_id and
                                       seq = 0
                            ),
                            404  -- account does not exists
                        )
                    else 404  -- account does not exists
                end;

        exit;
    when gdscode unique_key_violation do
        /*
        Here we know the account exist.
        If the insert do not raise exception in the next round, it will overwrite status_code with 200
        or it will leave the status code as 422 meaning the account is bankrupt.
        */
        status_code = 422;
    end
end!

set term ;!


insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (1, 0, 0, -100000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (2, 0, 0, -80000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (3, 0, 0, -1000000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (4, 0, 0, -10000000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
insert into transaction (account_id, seq, balance, overdraft, val, description, datetime) values (5, 0, 0, -500000, 0, '', timestamp '0001-01-01 00:00:00 GMT+0');
