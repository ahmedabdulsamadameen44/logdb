// Portions of this implementation were developed with
// assistance from Claude (Anthropic)
 
document.addEventListener("DOMContentLoaded", function () {
    const modeSelect = document.getElementById("mode-select");
    const countFieldRow = document.getElementById("count-field-row");
    const countFieldSelect = document.getElementById("count-field");
    const groupBySection = document.getElementById("group-by-section");
    const groupByField = document.getElementById("group-by-field");
    const groupBySort = document.getElementById("group-by-sort");
 
    const conditionsContainer = document.getElementById("conditions-container");
    const conditionTemplate = document.getElementById("condition-template");
    const addConditionBtn = document.getElementById("add-condition");
 
    const preview = document.getElementById("query-preview");
    const queryForm = document.getElementById("query-form");
    const queryInput = document.getElementById("query-input");
 
    function updateModeVisibility() {
        const isCount = modeSelect.value === "count";
        countFieldRow.hidden = !isCount;
        groupBySection.hidden = !isCount;
        if (!isCount) {
            groupByField.value = "";
            groupBySort.value = "";
        }
        buildQuery();
    }
 
    function addConditionRow() {
        const clone = conditionTemplate.content.cloneNode(true);
        const row = clone.querySelector(".condition-row");
 
        row.querySelector(".remove-condition").addEventListener("click", function () {
            row.remove();
            buildQuery();
        });
 
        row.querySelectorAll("select, input").forEach(function (el) {
            el.addEventListener("input", buildQuery);
        });
 
        conditionsContainer.appendChild(clone);
        buildQuery();
    }
 
    function buildQuery() {
        let parts = [];
 
        if (modeSelect.value === "count") {
            parts.push("SELECT COUNT " + countFieldSelect.value);
        } else {
            parts.push("SELECT *");
        }
 
        const rows = conditionsContainer.querySelectorAll(".condition-row");
        if (rows.length > 0) {
            let conditions = [];
            rows.forEach(function (row) {
                const field = row.querySelector(".cond-field").value;
                const op = row.querySelector(".cond-op").value;
                const value = row.querySelector(".cond-value").value.trim();
                if (value !== "") {
                    conditions.push(field + " " + op + " " + value);
                }
            });
            if (conditions.length > 0) {
                parts.push("WHERE " + conditions.join(" AND "));
            }
        }
 
        if (modeSelect.value === "count" && groupByField.value !== "") {
            let groupClause = "GROUP BY " + groupByField.value;
            if (groupBySort.value !== "") {
                groupClause += " " + groupBySort.value;
            }
            parts.push(groupClause);
        }
 
        const queryText = parts.join(" ");
        preview.textContent = queryText;
        queryInput.value = queryText;
        document.getElementById("save-query-input").value = queryText;
    }
 
    modeSelect.addEventListener("change", updateModeVisibility);
    addConditionBtn.addEventListener("click", addConditionRow);
    groupByField.addEventListener("change", buildQuery);
    groupBySort.addEventListener("change", buildQuery);
 
    queryForm.addEventListener("submit", function () {
        queryInput.value = preview.textContent;
    });
 
    // start with one condition row and correct initial visibility
    addConditionRow();
    updateModeVisibility();
});
 