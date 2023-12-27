class Quest {
    foreign static require_transport()
    foreign static wait_seconds()
    foreign static is_task_possible(task)
    foreign static invalid_task
    foreign static pay_money(ammount)
    foreign static pay_item(kind)
    foreign static pay_reputation(ammount)
    
    static ensure_possible(fn) {
        var res = fn.call()
        var counter = 0
        while (!is_quest_possible(quest)) {
            if (counter > 1000) {
                return invalid_task
            }
            res = fn.call()
        }
        return res
    }
}