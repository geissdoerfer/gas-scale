#!/usr/bin/env python3
"""
Generate bcrypt password hash for database initialization.
Usage: python generate_password_hash.py <password>
"""
import sys
from passlib.context import CryptContext

# Same configuration as in web-api/src/auth.py
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

def generate_hash(password: str) -> str:
    """Generate bcrypt hash for a password."""
    return pwd_context.hash(password)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_password_hash.py <password>")
        print("\nExample:")
        print("  python generate_password_hash.py admin123")
        sys.exit(1)

    password = sys.argv[1]
    hash_value = generate_hash(password)

    print(f"\nPassword: {password}")
    print(f"Hash: {hash_value}")
    print("\nYou can use this hash in your database/init.sql file.")
