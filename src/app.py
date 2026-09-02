import os
import bridge
from cs50 import SQL
from flask import Flask, flash, redirect, render_template, request, session
from flask_session import Session
from werkzeug.security import check_password_hash, generate_password_hash
from helpers import  login_required, apology

# Configure application
app = Flask(__name__)

# Configure session to use filesystem (instead of signed cookies)
app.config["SESSION_PERMANENT"] = False
app.config["SESSION_TYPE"] = "filesystem"
Session(app)

# Configure CS50 Library to use SQLite database
db = SQL("sqlite:///logdb.db")


@app.after_request
def after_request(response):
    """Ensure responses aren't cached"""
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate"
    response.headers["Expires"] = 0
    response.headers["Pragma"] = "no-cache"
    return response





@app.route("/login", methods=["GET", "POST"])
def login():
    """Log user in"""

    # Forget any user_id
    session.clear()

    # User reached route via POST (as by submitting a form via POST)
    if request.method == "POST":
        # Ensure username was submitted
        if not request.form.get("username"):
            return apology("must provide username", 400)

        # Ensure password was submitted
        elif not request.form.get("password"):
            return apology("must provide password", 400)

        # Query database for username
        rows = db.execute(
            "SELECT * FROM users WHERE username = ?", request.form.get("username")
        )

        # Ensure username exists and password is correct
        if len(rows) != 1 or not check_password_hash(
            rows[0]["hash"], request.form.get("password")
        ):
            return apology("invalid username and/or password", 400)

        # Remember which user has logged in
        session["user_id"] = rows[0]["id"]

        # Redirect user to home page
        return redirect("/")

    # User reached route via GET (as by clicking a link or via redirect)
    else:
        return render_template("login.html")




@app.route("/change" , methods = ["GET" , "POST"])
@login_required
def change():
    if request.method == "GET" :
        return render_template("change_get.html")



    else :
        user_id = session ["user_id"]
        current_password = request.form.get("current_password")
        same_current_password = request.form.get("same_current_password")
        new_password = request.form.get("new_password")
        same_new_password = request.form.get("same_new_password")
        if not user_id or not current_password or not same_current_password or not new_password or not same_new_password:
            return apology("MAKE SURE TO INPUT IN EVERY SPACE",400)
        if current_password != same_current_password :
            return apology("MUST BE SAME PASSWORD",400)
        if same_new_password != new_password :
            return apology("MUST BE SAME NEW PASSWORD",400)
        if current_password == new_password :
            return apology("SAME PASSWORD",400)


        current_password_db = db.execute("SELECT hash FROM users WHERE id = ?" , user_id)
        if not check_password_hash(current_password_db [0] ["hash"] , current_password) :
            return apology("WRONG PASSWORD" , 400)
        new_password_hash = generate_password_hash(new_password)
        new_password_hash_in_db = db.execute("UPDATE users SET hash = ? WHERE id = ? " , new_password_hash , user_id)
        return redirect("/")




    
@app.route("/logout")
def logout():
    """Log user out"""

    # Forget any user_id
    session.clear()

    # Redirect user to login form
    return redirect("/")










@app.route("/register", methods=["GET", "POST"])
def register():
    """Register user"""
    if request.method == "GET":
        return render_template("register.html")


    else : 
        # get the username and password
        username_register = request.form.get("username_register")
        password_register = request.form.get("password_register")
        password_register_check = request.form.get("password_register_check")


        #insure username and password
        if not request.form.get("username_register"):
            return apology("must provide username", 400)

        elif not request.form.get("password_register"):
            return apology("must provide password", 400)

        elif password_register != password_register_check :
            return apology("must provide the same password", 400)


        # add the new data to the database 
        username_check = db.execute("SELECT id  FROM users WHERE username = ?", username_register)
        if username_check : 
            return apology("USERNAME is not avalable", 400)

            
        rows_register = db.execute("INSERT INTO users (username, hash) VALUES (?,?)" , username_register, generate_password_hash(password_register))
        # Remember which user has logged in
        session["user_id"] = rows_register
        # Redirect user to home page
        return redirect("/")


@app.route("/query", methods=["GET", "POST"])
@login_required
def query_route():
    if request.method == "GET":
        return render_template("query_get.html")
    else:
        query_string = request.form.get("query")
        if not query_string:
            return apology("NO QUERY", 400)
        result = bridge.run_query(query_string)
        if "error" in result:
            return apology(result["error"], 400)
        return render_template("query_results.html", result=result)