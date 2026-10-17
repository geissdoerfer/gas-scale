"""
Tests for user management endpoints.
"""
import pytest


@pytest.mark.users
class TestListUsers:
    """Test list users endpoint."""

    def test_list_users_as_admin(self, client, auth_headers_admin, admin_user):
        """Test listing users as admin."""
        response = client.get("/users", headers=auth_headers_admin)
        assert response.status_code == 200
        users = response.json()

        assert isinstance(users, list)
        assert len(users) >= 1
        assert any(u["username"] == "admin" for u in users)

    def test_list_users_as_regular_user(self, client, auth_headers_user, regular_user):
        """Test that regular users cannot list users."""
        response = client.get("/users", headers=auth_headers_user)
        assert response.status_code == 403

    def test_list_users_no_auth(self, client):
        """Test listing users without authentication."""
        response = client.get("/users")
        assert response.status_code == 401


@pytest.mark.users
class TestCreateUser:
    """Test create user endpoint."""

    def test_create_user_as_admin(self, client, auth_headers_admin):
        """Test creating a user as admin."""
        user_data = {
            "username": "newuser",
            "email": "newuser@test.com",
            "password": "password123",
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 201
        data = response.json()

        assert data["username"] == "newuser"
        assert data["email"] == "newuser@test.com"
        assert data["role"] == "user"
        assert "id" in data
        assert "password" not in data  # Password should not be returned

    def test_create_admin_user(self, client, auth_headers_admin):
        """Test creating an admin user."""
        user_data = {
            "username": "newadmin",
            "email": "newadmin@test.com",
            "password": "adminpass123",
            "role": "admin"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 201
        assert response.json()["role"] == "admin"

    def test_create_user_duplicate_username(self, client, auth_headers_admin, regular_user):
        """Test creating user with duplicate username."""
        user_data = {
            "username": "testuser",  # Already exists
            "email": "different@test.com",
            "password": "password123",
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 400

    def test_create_user_duplicate_email(self, client, auth_headers_admin, regular_user):
        """Test creating user with duplicate email."""
        user_data = {
            "username": "different",
            "email": "testuser@test.com",  # Already exists
            "password": "password123",
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 400

    def test_create_user_invalid_email(self, client, auth_headers_admin):
        """Test creating user with invalid email."""
        user_data = {
            "username": "newuser",
            "email": "not-an-email",
            "password": "password123",
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 422  # Validation error

    def test_create_user_short_password(self, client, auth_headers_admin):
        """Test creating user with too short password."""
        user_data = {
            "username": "newuser",
            "email": "newuser@test.com",
            "password": "short",  # Less than 8 characters
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_admin)
        assert response.status_code == 422

    def test_create_user_as_regular_user(self, client, auth_headers_user):
        """Test that regular users cannot create users."""
        user_data = {
            "username": "newuser",
            "email": "newuser@test.com",
            "password": "password123",
            "role": "user"
        }
        response = client.post("/users", json=user_data, headers=auth_headers_user)
        assert response.status_code == 403


@pytest.mark.users
class TestUpdateUser:
    """Test update user endpoint."""

    def test_update_user_as_admin(self, client, auth_headers_admin, regular_user):
        """Test updating a user as admin."""
        update_data = {
            "email": "updated@test.com",
            "role": "admin"
        }
        response = client.put(
            f"/users/{regular_user.id}",
            json=update_data,
            headers=auth_headers_admin
        )
        assert response.status_code == 200
        data = response.json()

        assert data["email"] == "updated@test.com"
        assert data["role"] == "admin"

    def test_update_user_password(self, client, auth_headers_admin, regular_user):
        """Test updating user password."""
        update_data = {
            "password": "newpassword123"
        }
        response = client.put(
            f"/users/{regular_user.id}",
            json=update_data,
            headers=auth_headers_admin
        )
        assert response.status_code == 200

        # Verify new password works
        login_response = client.post(
            "/auth/login",
            json={"username": "testuser", "password": "newpassword123"}
        )
        assert login_response.status_code == 200

    def test_update_nonexistent_user(self, client, auth_headers_admin):
        """Test updating non-existent user."""
        response = client.put(
            "/users/99999",
            json={"email": "test@test.com"},
            headers=auth_headers_admin
        )
        assert response.status_code == 404

    def test_update_user_as_regular_user(self, client, auth_headers_user, regular_user):
        """Test that regular users cannot update users."""
        response = client.put(
            f"/users/{regular_user.id}",
            json={"email": "new@test.com"},
            headers=auth_headers_user
        )
        assert response.status_code == 403


@pytest.mark.users
class TestDeleteUser:
    """Test delete user endpoint."""

    def test_delete_user_as_admin(self, client, auth_headers_admin, regular_user):
        """Test deleting a user as admin."""
        user_id = regular_user.id
        response = client.delete(f"/users/{user_id}", headers=auth_headers_admin)
        assert response.status_code == 200

        # Verify user is deleted
        list_response = client.get("/users", headers=auth_headers_admin)
        users = list_response.json()
        assert not any(u["id"] == user_id for u in users)

    def test_delete_nonexistent_user(self, client, auth_headers_admin):
        """Test deleting non-existent user."""
        response = client.delete("/users/99999", headers=auth_headers_admin)
        assert response.status_code == 404

    def test_delete_user_as_regular_user(self, client, auth_headers_user, admin_user):
        """Test that regular users cannot delete users."""
        response = client.delete(f"/users/{admin_user.id}", headers=auth_headers_user)
        assert response.status_code == 403
